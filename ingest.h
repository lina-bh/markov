#ifndef INGEST_H
#define INGEST_H

struct mmapped_file;
struct word_tbl;

void ingest(struct mmapped_file *mf, struct word_tbl *tbl);

#endif
