//
// Copyright RIME Developers
// Distributed under the BSD License
//
// simplistic sentence-making
//
// 2011-10-06 GONG Chen <chen.sst@gmail.com>
//
#include <algorithm>
#include <functional>
#include <rime/candidate.h>
#include <rime/config.h>
#include <rime/dict/vocabulary.h>
#include <rime/gear/grammar.h>
#include <rime/gear/poet.h>

namespace rime {

inline static Grammar* create_grammar(Config* config) {
  if (auto* grammar = Grammar::Require("grammar")) {
    return grammar->Create(config);
  }
  return nullptr;
}

Poet::Poet(const Language* language, Config* config, Compare compare)
    : language_(language),
      grammar_(create_grammar(config)),
      compare_(compare) {}

Poet::~Poet() {}

bool Poet::LeftAssociateCompare(const Sentence& one, const Sentence& other) {
  return one.weight() < other.weight() || (  // left associate if even
      one.weight() == other.weight() && (
          one.size() > other.size() || (  // less components is more favorable
              one.size() == other.size() &&
              std::lexicographical_compare(one.syllable_lengths().begin(),
                                           one.syllable_lengths().end(),
                                           other.syllable_lengths().begin(),
                                           other.syllable_lengths().end()))));
}

an<Sentence> Poet::MakeSentence(const WordGraph& graph,
                                size_t total_length,
                                const string& preceding_text) {
  // TODO: save more intermediate sentence candidates
  map<int, an<Sentence>> sentences;
  sentences[0] = New<Sentence>(language_);
  // dynamic programming
  for (const auto& w : graph) {
    size_t start_pos = w.first;
    DLOG(INFO) << "start pos: " << start_pos;
    if (sentences.find(start_pos) == sentences.end())
      continue;
    for (const auto& x : w.second) {
      size_t end_pos = x.first;
      if (start_pos == 0 && end_pos == total_length)
        continue;  // exclude single words from the result
      DLOG(INFO) << "end pos: " << end_pos;
      bool is_rear = end_pos == total_length;
      const DictEntryList& entries(x.second);
      for (const auto& entry : entries) {
        auto new_sentence = New<Sentence>(*sentences[start_pos]);
        new_sentence->Extend(
            *entry, end_pos, is_rear, preceding_text, grammar_.get());
        if (sentences.find(end_pos) == sentences.end() ||
            compare_(*sentences[end_pos], *new_sentence)) {
          DLOG(INFO) << "updated sentences " << end_pos << ") with "
                     << new_sentence->text() << " weight: "
                     << new_sentence->weight();
          sentences[end_pos] = std::move(new_sentence);
        }
      }
    }
  }
  if (sentences.find(total_length) == sentences.end())
    return nullptr;
  else
    return sentences[total_length];
}

}  // namespace rime
