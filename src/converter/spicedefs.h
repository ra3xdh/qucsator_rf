#ifndef SPICEDEFS_H
#define SPICEDEFS_H

static struct property_t req_CORE[] = {
  { "A", 1, { 26.0, ((char *) -1) }, { '[', 0, 0, ']',
    { ((char *) 0) } } },
  { "K", 1, { 16.0, ((char *) -1) }, { '[', 0, 0, '.',
    { ((char *) 0) } } },
  { "C", 1, { 0.08, ((char *) -1) }, { '[', 0, 0, '.',
    { ((char *) 0) } } },
  { "MS", 1, { 300e3, ((char *) -1) }, { '[', 0, 0, '.',
    { ((char *) 0) } } },
  { "alpha", 1, { 1e-4, ((char *) -1) }, { '[', -273.15, 0, '.',
    { ((char *) 0) } } },
  { ((char *) 0), 1, { 0, ((char *) -1) }, { '.', 0, 0, '.',
    { ((char *) 0) } } }
};


static struct property_t opt_CORE[] = {
  { ((char *) 0), 1, { 0, ((char *) -1) }, { '.', 0, 0, '.',
    { ((char *) 0) } } }
};


static struct define_t def_CORE = {
  "CORE", 2, 0, 0, 0, req_CORE, opt_CORE };


struct define_t extra_definition_available[] =
{
  def_CORE
};

#endif
