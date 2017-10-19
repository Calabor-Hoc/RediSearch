#ifndef FRAGMENTER_H
#define FRAGMENTER_H

#include <stdlib.h>
#include <stdint.h>
#include <sys/uio.h>
#include "util/array.h"
#include "stemmer.h"
#include "redisearch.h"
#include "stopwords.h"
#include "byte_offsets.h"

typedef struct {
  uint32_t tokPos;
  uint32_t bytePos;
  uint32_t termId;
  float score;
} FragmentTerm;

typedef struct {
  RSByteOffsetIterator *byteIter;
  RSOffsetIterator *offsetIter;
  RSQueryTerm *curMatchRec;
  uint32_t curTokPos;
  uint32_t curByteOffset;
  FragmentTerm tmpTerm;
} FragmentTermIterator;

int FragmentTermIterator_Next(FragmentTermIterator *iter, FragmentTerm **termInfo);
void FragmentTermIterator_InitOffsets(FragmentTermIterator *iter, RSByteOffsetIterator *bytesIter,
                                      RSOffsetIterator *offIter);

typedef struct {
  // Position in current fragment (bytes)
  uint32_t offset;

  // Length of the token. This might be a stem, so not necessarily similar to termId
  uint16_t len;

  // Index into FragmentList::terms
  uint16_t termId;
} TermLoc;

typedef struct Fragment {
  const char *buf;
  uint32_t len;

  // (token-wise) position of the last matched token
  uint32_t lastMatchPos;

  // How many tokens are in this fragment
  uint32_t totalTokens;

  // How many _matched_ tokens are in this fragment
  uint32_t numMatches;

  // Inverted ranking (from 0..n) in the score array. A lower number means a higher score
  uint32_t scoreRank;

  // Position within the array of fragments
  uint32_t fragPos;

  // Score calculated from the number of matches
  float score;
  Array termLocs;  // TermLoc
} Fragment;

typedef struct {
  // Array of fragments
  Array frags;

  // Array of indexes (in frags), sorted by score
  uint32_t *sortedFrags;

  // Scratch space, used internally
  uint32_t *scratchFrags;

  // Number of fragments
  uint32_t numFrags;

  // Number of tokens since last match. Used in determining 'context ratio'
  uint32_t numToksSinceLastMatch;

  const char *doc;
  uint32_t docLen;

  // Maximum allowable distance between relevant terms to be called a 'fragment'
  uint16_t maxDistance;

  // Average word size. Used when determining context.
  uint8_t estAvgWordSize;
} FragmentList;

static inline void FragmentList_Init(FragmentList *fragList, uint16_t maxDistance,
                                     uint8_t estWordSize) {
  fragList->doc = NULL;
  fragList->docLen = 0;
  fragList->numFrags = 0;
  fragList->maxDistance = maxDistance;
  fragList->estAvgWordSize = estWordSize;
  fragList->sortedFrags = NULL;
  fragList->scratchFrags = NULL;
  Array_Init(&fragList->frags);
}

static inline size_t FragmentList_GetNumFrags(const FragmentList *fragList) {
  return ARRAY_GETSIZE_AS(&fragList->frags, Fragment);
}

static const Fragment *FragmentList_GetFragments(const FragmentList *fragList) {
  return ARRAY_GETARRAY_AS(&fragList->frags, const Fragment *);
}

#define FRAGMENT_TERM(buf_, len_, score_) \
  { .tok = buf_, .len = len_, .score = score_ }
/**
 * A single term to use for searching. Used when fragmenting a buffer
 */
typedef struct {
  const char *tok;
  size_t len;
  float score;
} FragmentSearchTerm;

#define DOCLEN_NULTERM ((size_t)-1)

/**
 * Split a document into a list of fragments.
 * - doc is the document to split
 * - numTerms is the number of terms to search for. The terms themselves are
 *   not searched, but each Fragment needs to have this memory made available.
 *
 * Returns a list of fragments.
 */
void FragmentList_FragmentizeBuffer(FragmentList *fragList, const char *doc, Stemmer *stemmer,
                                    StopWordList *stopwords, const FragmentSearchTerm *terms,
                                    size_t numTerms);

void FragmentList_FragmentizeIter(FragmentList *fragList, const char *doc, size_t docLen,
                                  FragmentTermIterator *iter);

typedef struct {
  const char *openTag;
  const char *closeTag;
} HighlightTags;

void FragmentList_Free(FragmentList *frags);

/** Highlight matches the entire document, returning a series of IOVs */
void FragmentList_HighlightWholeDocV(const FragmentList *fragList, const HighlightTags *tags,
                                     Array *iovs);

/** Highlight matches the entire document, returning it as a freeable NUL-terminated buffer */
char *FragmentList_HighlightWholeDocS(const FragmentList *fragList, const HighlightTags *tags);

/**
 * Return fragments by their score. The highest ranked fragment is returned fist
 */
#define HIGHLIGHT_ORDER_SCORE 0x01

/**
 * Return fragments by their order in the document. The fragment with the lowest
 * position is returned first.
 */
#define HIGHLIGHT_ORDER_POS 0x02

/**
 * First select the highest scoring elements and then sort them by position
 */
#define HIGHLIGHT_ORDER_SCOREPOS 0x03
/**
 * Highlight fragments for each document.
 *
 * - contextSize is the size of the surrounding context, in estimated words,
 * for each returned fragment. The function will use this as a hint.
 *
 * - iovBufList is an array of buffers. Each element corresponds to a fragment,
 * and the fragments are always returned in order.
 *
 * - niovs If niovs is less than the number of fragments, then only the first
 * <niov> fragments are returned.
 *
 * - order is one of the HIGHLIGHT_ORDER_ constants. See their documentation
 * for more details
 *
 */
void FragmentList_HighlightFragments(FragmentList *fragList, const HighlightTags *tags,
                                     size_t contextSize, Array *iovBufList, size_t niovs,
                                     int order);

void FragmentList_Dump(const FragmentList *fragList);

#endif