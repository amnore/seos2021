int isspace(int ch) {
  switch (ch) {
  case ' ':
  case '\f':
  case '\n':
  case '\r':
  case '\t':
  case '\v':
    return 1;
  default:
    return 0;
  }
}

int isdigit(int ch) { return '0' <= ch && ch <= '9'; }
