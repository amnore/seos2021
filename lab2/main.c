#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

#define alloc(type, n) ((type *)_alloc(sizeof(type), n))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define COLOR_RED "\033[31m"
#define COLOR_NONE "\033[0m"

typedef struct fstree {
  enum { TYPE_FILE, TYPE_DIR } type;
  char name[13];
  size_t size;
  char *data;
  struct {
    size_t ndirs;
    size_t nfiles;
    size_t nentries;
    struct fstree *entries;
  } dir;
} fstree;

typedef struct bpb {
  uint8_t _useless0[11];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sector_nr;
  uint8_t fat_nr;
  uint16_t root_entries_count;
  uint16_t total_sectors;
  uint8_t _useless1[1];
  uint16_t fat_sectors;
  uint8_t _useless2[8];
  uint32_t total_sectors_32;
} __attribute__((packed)) bpb;
static_assert(sizeof(bpb) == 36, "");

enum file_attr {
  ATTR_READ_ONLY = 0x1,
  ATTR_HIDDEN = 0x2,
  ATTR_SYSTEM = 0x4,
  ATTR_VOLUME_ID = 0x8,
  ATTR_DIRECTORY = 0x10,
  ATTR_ARCHIVE = 0x20,
  ATTR_LONG_NAME = (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
};

typedef struct file_ent {
  char short_name[11];
  uint8_t attr;
  uint8_t _useless0[8];
  uint16_t cluster_high;
  uint8_t _useless1[4];
  uint16_t cluster_low;
  uint32_t size;
} __attribute__((packed)) file_ent;
static_assert(sizeof(file_ent) == 32, "");

static fstree root;
static void *partition, *fat, *data;
static file_ent *root_dir;
static size_t cluster_size, root_nentries, nclusters;

void myputs(int fd, const char *str, size_t len);

static void *_alloc(size_t sz, size_t n) {
  void *p = calloc(n, sz);
  assert(p);
  return p;
}

static void assert_perror(bool cond) {
  if (!cond) {
    perror("fat12ls");
    exit(-1);
  }
}

static void print_args(int argc, char **argv) {
  for (int i = 0; i < argc; i++) {
    int len = strlen(argv[i]);
    myputs(2, argv[i], len);
  }
}

static void myputstr(int fd, const char *str) {
  size_t len = strlen(str);
  myputs(fd, str, len);
}

static void copy_filename(char *dst, char src[11]) {
  int i = 0;
  while (i < 8 && src[i] != ' ')
    i++;
  memcpy(dst, src, i);

  if (src[8] == ' ')
    return;
  dst[i] = '.';

  int j = 0;
  while (j < 3 && src[j + 8] != ' ')
    j++;
  memcpy(dst + i + 1, src + 8, j);
}

static bool cluster_isvalid(size_t n) { return 2 <= n && n <= nclusters; }

static size_t cluster_next(size_t n) {
  size_t offset = n + n / 2;

  return (n & 1) ? (*(uint16_t *)(fat + offset) >> 4)
                 : (*(uint16_t *)(fat + offset) & 0xfff);
}

static void read_entry(fstree *node, file_ent *ent) {
  node->type = (ent->attr & ATTR_DIRECTORY) ? TYPE_DIR : TYPE_FILE;
  copy_filename(node->name, ent->short_name);

  if (node->type == TYPE_FILE) {
    node->size = ent->size;
  } else {
    node->size = 0;
    for (size_t n = ent->cluster_low; cluster_isvalid(n); n = cluster_next(n))
      node->size += cluster_size;
  }

  size_t nbytes = 0;
  char *buf = node->data = alloc(char, node->size);
  for (size_t cluster = ent->cluster_low;
       cluster_isvalid(cluster) && nbytes < node->size;
       cluster = cluster_next(cluster)) {
    size_t ncopy = min(cluster_size, node->size - nbytes);
    memcpy(buf, (cluster - 2) * cluster_size + data, ncopy);
    nbytes += ncopy;
    buf += ncopy;
  }
}

static void read_dir_recursive(fstree *dir, file_ent *ents, size_t nentries) {
  size_t nfiles = 0, ndirs = 0;

  size_t total = 0;
  dir->dir.entries = alloc(fstree, nentries);
  for (size_t i = 0; i < nentries; i++) {
    if (*(char *)&ents[i] == 0)
      break;

    if (ents[i].short_name[0] == (char)0xe5)
      continue;

    if (ents[i].attr == ATTR_LONG_NAME)
      continue;

    fstree *node = &dir->dir.entries[total];
    total++;
    read_entry(node, &ents[i]);

    if (node->type == TYPE_DIR && strcmp(node->name, ".") != 0 &&
        strcmp(node->name, "..") != 0) {
      ndirs++;
      read_dir_recursive(node, (file_ent *)node->data, node->size / 32);
    } else {
      nfiles++;
    }
  }

  dir->dir.ndirs = ndirs;
  dir->dir.nfiles = nfiles;
  dir->dir.nentries = total;
}

static void read_metadata() {
  bpb *bpb = partition;
  fat = partition + bpb->bytes_per_sector * bpb->reserved_sector_nr;
  data = partition +
         bpb->bytes_per_sector *
             (bpb->reserved_sector_nr + bpb->fat_sectors * bpb->fat_nr +
              (bpb->root_entries_count * 32 + bpb->bytes_per_sector - 1) /
                  bpb->bytes_per_sector);
  root_dir = partition +
             bpb->bytes_per_sector *
                 (bpb->reserved_sector_nr + bpb->fat_nr * bpb->fat_sectors);
  cluster_size = bpb->sectors_per_cluster * bpb->bytes_per_sector;
  nclusters =
      ((bpb->total_sectors == 0 ? bpb->total_sectors_32 : bpb->total_sectors) -
       bpb->reserved_sector_nr - bpb->fat_sectors * bpb->fat_nr) /
      bpb->sectors_per_cluster;
  root_nentries = bpb->root_entries_count;

  root.name[0] = '\0';
  root.type = TYPE_DIR;
}

static const char *mychr(const char *s, char ch) {
  const char *p = strchr(s, ch);
  return p ? p : s + strlen(s);
}

static fstree *find_file(const char *path) {
  if (path[0] != '/') {
    goto err_notabs;
  }

  fstree *file = &root;
  const char *s = path + 1, *sep = mychr(s + 1, '/');
  while (*s) {
    if (file->type == TYPE_FILE) {
      goto err_nofile;
    }

    int i = 0;
    for (; i < file->dir.nentries; i++) {
      fstree *ent = &file->dir.entries[i];
      if (strncmp(ent->name, s, sep - s) == 0 && ent->name[sep - s] == '\0') {
        break;
      }
    }

    if (i == file->dir.nentries) {
      goto err_nofile;
    }
    file = &file->dir.entries[i];

    if (!*sep) {
      break;
    }
    s = sep + 1;
    sep = mychr(sep + 1, '/');
  }

  if (*(sep - 1) == '/' && file->type != TYPE_DIR)
    goto err_notdir;
  return file;

err_nofile:
  myputstr(2, "No such file or directory\n");
  return NULL;

err_notdir:
  myputstr(2, "Not a directory\n");
  return NULL;

err_notabs:
  myputstr(2, "Not an absolute path\n");
  return NULL;
}

static void list_files(char *path, fstree *node, bool list) {
  if (node->type == TYPE_FILE) {
    myputstr(1, path);
    if (list) {
      char buf[32];
      snprintf(buf, sizeof(buf), " %lu", node->size);
      myputstr(1, buf);
    }
    myputstr(1, "\n");
    return;
  }

  myputstr(1, path);
  if (list) {
    char buf[32];
    snprintf(buf, sizeof(buf), " %lu %lu", node->dir.ndirs, node->dir.nfiles);
    myputstr(1, buf);
  }
  myputstr(1, ":\n");

  for (int i = 0; i < node->dir.nentries; i++) {
    fstree *ent = &node->dir.entries[i];
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%s%s  ",
             ent->type == TYPE_DIR ? COLOR_RED : "", ent->name,
             ent->type == TYPE_DIR ? COLOR_NONE : "");
    myputstr(1, buf);

    if (list) {
      if (ent->type == TYPE_DIR) {
        if (strcmp(ent->name, ".") != 0 && strcmp(ent->name, "..") != 0)
          snprintf(buf, sizeof(buf), "%lu %lu", ent->dir.ndirs,
                   ent->dir.nfiles);
        else {
          buf[0] = '\0';
        }
      } else {
        snprintf(buf, sizeof(buf), "%lu", ent->size);
      }
      strcat(buf, "\n");
      myputstr(1, buf);
    }
  }
  myputstr(1, "\n");

  int len = strlen(path);
  for (int i = 0; i < node->dir.nentries; i++) {
    fstree *ent = &node->dir.entries[i];
    if (ent->type == TYPE_DIR && strcmp(ent->name, ".") != 0 &&
        strcmp(ent->name, "..") != 0) {
      strcat(path, ent->name);
      strcat(path, "/");
      list_files(path, ent, list);
      path[len] = '\0';
    }
  }
}

static void cmd_ls(int argc, char **argv) {
  bool list = false;
  char opt;
  const char *path = NULL;

  optind = 0;
  opterr = 0;
  while ((opt = getopt(argc, argv, "-l")) != -1) {
    switch (opt) {
    case 'l':
      list = true;
      break;
    case 1:
      if (path) {
        myputstr(2, "Multiple paths specified\n");
        return;
      }
      path = optarg;
      break;
    default:
      myputstr(2, "Invalid option\n");
      return;
    }
  }

  if (!path) {
    path = "/";
  }

  fstree *node = find_file(path);
  if (!node) {
    return;
  }

  char buf[256];
  size_t len = strlen(path);
  memcpy(buf, path, len);
  if (path[len - 1] == '/' || node->type == TYPE_FILE) {
    buf[len] = '\0';
  } else {
    buf[len] = '/';
    buf[len + 1] = '\0';
  }
  list_files(buf, node, list);
}

static void cmd_cat(int argc, char **argv) {
  if (argc != 2) {
    myputstr(2, "cat <file>\n");
  }

  fstree *node = find_file(argv[1]);
  if (!node) {
    return;
  }

  if (node->type == TYPE_DIR) {
    myputstr(2, "Not a regular file\n");
    return;
  }

  myputs(1, node->data, node->size);
  myputstr(1, "\n");
}

static void cmd_q(int argc, char **argv) { exit(0); }

static void cmd_invalid(int argc, char **argv) {
  myputstr(2, "Invalid command\n");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    myputstr(2, "Usage: lab2 <img>");
    return -1;
  }

  int fd = open(argv[1], O_RDONLY);
  assert_perror(fd >= 0);

  struct stat st;
  assert_perror(fstat(fd, &st) == 0);
  assert_perror((partition = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd,
                                  0)) != MAP_FAILED);

  read_metadata();
  read_dir_recursive(&root, root_dir, root_nentries);

  char *line;
  using_history();
  while ((line = readline("> ")) != NULL) {
    add_history(line);

    char *args[16], *sep = line;
    int nargs = 0;
    while (nargs < 16) {
      args[nargs] = strsep(&sep, " ");

      if (!args[nargs])
        break;

      if (strlen(args[nargs]) != 0)
        nargs++;
    }

    if (strcmp(args[0], "ls") == 0) {
      cmd_ls(nargs, args);
    } else if (strcmp(args[0], "cat") == 0) {
      cmd_cat(nargs, args);
    } else if (strcmp(args[0], "q") == 0) {
      cmd_q(nargs, args);
    } else {
      cmd_invalid(nargs, args);
    }

    free(line);
  }

  return 0;
}
