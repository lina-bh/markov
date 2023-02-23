#ifndef VOMIT_H
#define VOMIT_H

struct mmapped_file;
struct word_tbl;

void vomit(struct mmapped_file *mf, struct word_tbl *tbl, int urandom_fd);

#endif
