// Copyright 2017-2022 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VERIBLE_VERILOG_FLOW_TREE_H_
#define VERIBLE_VERILOG_FLOW_TREE_H_

#include <bitset>
#include <map>
#include <stack>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "common/lexer/token_stream_adapter.h"
#include "verilog/parser/verilog_token_enum.h"

namespace verilog {

using BitSet = std::bitset<128>;
using TokenSequenceConstIterator = verible::TokenSequence::const_iterator;

struct ConditionalBlock {
  TokenSequenceConstIterator ifdef_iterator;
  TokenSequenceConstIterator ifndef_iterator;
  std::vector<TokenSequenceConstIterator> elsif_iterators;
  TokenSequenceConstIterator else_iterator;
  TokenSequenceConstIterator endif_iterator;
};

struct Variant {
  // Contains the token sequence of the variant.
  verible::TokenSequence sequence;

  // The i-th bit in "macros_mask" is 1 when the macro (with ID = i) is assumed
  // to be defined, o.w. it is assumed to be undefined.
  BitSet macros_mask;

  // The i-th bit in "assumed" is 1 when the macro (with ID = i) is visited or
  // assumed (either defined or not), o.w. it is not visited (its value doesn't
  // affect this variant).
  BitSet assumed;
};

// Receive a complete token sequence of one variant.
// variant_sequence: the generated variant token sequence.
using VariantReceiver = std::function<bool(const Variant &variant)>;

// FlowTree class builds the control flow graph of a tokenized System-Verilog
// source code. Furthermore, enabling doing the following queries on the graph:
// 1) Generating all the possible variants (provided via a callback function).
// 2) TODO(karimtera): uniquely identify a variant with a bitset.
class FlowTree {
 public:
  explicit FlowTree(verible::TokenSequence source_sequence)
      : source_sequence_(std::move(source_sequence)){};

  // Generates all possible variants.
  absl::Status GenerateVariants(const VariantReceiver &receiver);

 private:
  // Constructs the control flow tree by adding the tree edges in edges_.
  absl::Status GenerateControlFlowTree();

  // Traveses the tree in a depth first manner.
  absl::Status DepthFirstSearch(const VariantReceiver &receiver,
                                TokenSequenceConstIterator current_node);

  // Checks if the iterator points to a conditonal directive (`ifdef/ifndef...).
  static bool IsConditional(TokenSequenceConstIterator iterator);

  // Adds all edges withing a conditional block.
  absl::Status AddBlockEdges(const ConditionalBlock &block);

  // The tree edges which defines the possible next childs of each token in
  // source_sequence_.
  std::map<TokenSequenceConstIterator, std::vector<TokenSequenceConstIterator>>
      edges_;

  // Extracts the conditional macro checked.
  static absl::Status MacroFollows(
      TokenSequenceConstIterator conditional_iterator);

  // Adds macro to conditional_macro_id_ map.
  absl::Status AddMacroOfConditionalToMap(
      TokenSequenceConstIterator conditional_iterator);

  int GetMacroIDOfConditional(TokenSequenceConstIterator conditional_iterator);

  // Holds all of the conditional blocks.
  std::vector<ConditionalBlock> if_blocks_;

  // The original source code lexed token seqouence.
  const verible::TokenSequence source_sequence_;

  // Current variant being generated by DepthFirstSearch.
  Variant current_variant_;

  // A flag that determines if the VariantReceiver returned 'false'.
  // By default: it assumes VariantReceiver wants more variants.
  bool wants_more_ = 1;

  // Mapping each conditional macro to an integer ID,
  // to use it later as a bit offset.
  std::map<absl::string_view, int> conditional_macro_id_;

  // Number of macros appeared in `ifdef/`ifndef/`elsif.
  int conditional_macros_counter_ = 0;
};

}  // namespace verilog

#endif  // VERIBLE_VERILOG_FLOW_TREE_H_
