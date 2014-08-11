#include "rtree.h"
#include <memory>
#include <cmath>
#include <string>

namespace {
using std::unique_ptr;
using std::vector;
using std::ceil;
using std::string;
using std::to_string;

/// debug - checks the branch has only leaves or nodes, not both
bool validate_branch(const rtree::branch* branch) {
  auto leaves = 0u;
  auto branches = 0u;
  for(const auto& child : branch->children) {
    const bool is_leaf = dynamic_cast<rtree::leaf*>(child.get()) != nullptr;
    if(is_leaf)
      ++leaves;
    else
      ++branches;
  }
  if(leaves != 0 && branches != 0)
    return false;
  return true;
}
}

namespace rtree {
const string indent_str = "  ";

string get_indent(const size_t indent) {
  string s;
  for(size_t i = 0, end = indent; i != end; ++i) 
    s += indent_str;
  return s;
}

size_t node::next_id = 0; // debug and TOTALLY not threadsafe

void tree::insert(const data& d) {
  if(!root) {
    root = unique_ptr<leaf>(new leaf(this, nullptr));
    root->bounding_box = d.r;
    root->parent = nullptr;
  }
  root->insert(d);
}

/// \todo reduce unnecessary bounding box calculations
void branch::calculate_bounding_box() {
  validate_branch(this);
  bounding_box.top = ord_t_max;
  bounding_box.left = ord_t_max;
  bounding_box.right = ord_t_min;
  bounding_box.bottom = ord_t_min;

  for(auto child = children.begin(), end = children.end(); child != end; ++child) {
    if((*child)->bounding_box.top < bounding_box.top)
      bounding_box.top = (*child)->bounding_box.top;
    if((*child)->bounding_box.left < bounding_box.left)
      bounding_box.left = (*child)->bounding_box.left;
    if((*child)->bounding_box.right > bounding_box.right)
      bounding_box.right = (*child)->bounding_box.right;
    if((*child)->bounding_box.bottom > bounding_box.bottom)
      bounding_box.bottom = (*child)->bounding_box.bottom;
  }
  if(!!parent)
    parent->calculate_bounding_box();
  validate_branch(this);
}


void leaf::calculate_bounding_box() {
  bounding_box.top = ord_t_max;
  bounding_box.left = ord_t_max;
  bounding_box.right = ord_t_min;
  bounding_box.bottom = ord_t_min;

  for(auto child = children.begin(), end = children.end(); child != end; ++child) {
    if((*child)->r.top < bounding_box.top)
      bounding_box.top = (*child)->r.top;
    if((*child)->r.left < bounding_box.left)
      bounding_box.left = (*child)->r.left;
    if((*child)->r.right > bounding_box.right)
      bounding_box.right = (*child)->r.right;
    if((*child)->r.bottom > bounding_box.bottom)
      bounding_box.bottom = (*child)->r.bottom;
  }
  if(!!parent)
    parent->calculate_bounding_box();
}

ord_t node::expansion(const rect& r) {
  auto expansion = (ord_t) 0;
  if(bounding_box.top > r.top)
    expansion += bounding_box.top - r.top;
  if(bounding_box.bottom < r.bottom)
    expansion += r.bottom - bounding_box.bottom;
  if(bounding_box.left > r.left)
    expansion += bounding_box.left - r.left;
  if(bounding_box.right > r.right)
    expansion += bounding_box.right - r.right;
  return expansion;
}

std::vector<std::unique_ptr<node>>::iterator branch::smallest_expansion_child(const rect& r) {
  auto smallest_child = children.end();
  {
    auto smallest_expansion = ord_t_max;
    for(auto child = children.begin(), end = children.end(); child != end; ++child) {
      const auto child_expansion = (*child)->expansion(r);
      if(child_expansion > smallest_expansion)
	continue;
      smallest_expansion = child_expansion;
      smallest_child = child;
      if(smallest_expansion == 0)
	break;
    }
  }
  return smallest_child;
}

bool branch::insert(const data& d) {
  validate_branch(this);

  (*smallest_expansion_child(d.r))->insert(d);
  validate_branch(this);
}

bool leaf::insert(const data& d) {
  if(children.size() < max_fill) {
    children.emplace_back(unique_ptr<data>(new data(d)));
    calculate_bounding_box();
    return true;
  }
  split(d);
}

/// used by ::split(). 
/// if splitting a root node, this makes a new root.
void node::reroot() {
  if(!parent) {
    if(rtree->root.get() != this)
      int debug = 42;

    // !parent => root == this
    auto new_root = unique_ptr<node>(new branch(rtree, nullptr));
    auto new_root_raw = (branch*) new_root.get();

    new_root_raw->add(rtree->root); // rtree.root == this
    rtree->root = move(new_root); // new_root.add should have stolen the root pointer
    parent = (branch*) rtree->root.get();
  }
}

/// \todo make this use an intelligent algorithm
void leaf::split(const data& d) {
  const auto split_i = max_fill / 2 + 1;

  reroot();

  auto new_leaf = unique_ptr<node>(new leaf(rtree, parent));
  {
    auto new_leaf_raw = (leaf*) new_leaf.get();
    while(children.size() > split_i) {
      new_leaf_raw->children.push_back(move(children.back()));
      children.pop_back();
    }
    new_leaf_raw->children.emplace_back(unique_ptr<data>(new data(d)));
  }
  calculate_bounding_box();
  new_leaf->calculate_bounding_box();

  parent->add(new_leaf);
}

/// this should only be called by split(). Use insert() to add data normally. 
/// steals the passed unique_ptr's pointer
void branch::add(unique_ptr<node>& n) {
  validate_branch(this);

  //debug
  const bool n_leaf = dynamic_cast<leaf*>(n.get()) != nullptr;
  const bool leaves = children.empty() ? n_leaf : (dynamic_cast<leaf*>(children[0].get()) != nullptr);
  if(n_leaf != leaves)
    int debug = 42;

  if(children.size() < max_fill) {
    children.push_back(move(n));
    calculate_bounding_box();

    validate_branch(this);

    return;
  }
  split(n);

  validate_branch(this);
}

/// \todo use intelligent split algorithm
void branch::split(unique_ptr<node>& n) {
  validate_branch(this);
  const auto split_i = max_fill / 2;

  reroot();

  auto new_branch = unique_ptr<node>(new branch(rtree, parent));
  {
    auto new_branch_raw = (branch*) new_branch.get();
    while(children.size() > split_i) {
      new_branch_raw->children.push_back(move(children.back()));
      children.pop_back();
    }
    new_branch_raw->children.push_back(move(n));
  }

  calculate_bounding_box();
  new_branch->calculate_bounding_box();


  parent->add(new_branch);

  validate_branch(this);
}

string tree::str() {
  return "{ \"tree\" : \n" + root->str(0) + "\n}";
}

string rect::str() {
  return "["
    + to_string(top) + ", "
    + to_string(left) + ", "
    + to_string(bottom) + ", "
    + to_string(right) + "]";
}

string data::str() {
  return "{ \"key\" : " + to_string(key) + ", \"rectangle\" : " + r.str() + "}";
}

string leaf::str(const size_t indent) {
  auto s = get_indent(indent) + "{ \"type\" : \"leaf\", \"id\" : " + to_string(id) + ", \"bounding-box\" : " + bounding_box.str() + ", \"children\" : [ \n";
  for(const auto& child : children)
    s += get_indent(indent + 1) + child->str() + ", \n";
  s.erase(s.end() - 3); // remove last ', \n'
  s += get_indent(indent) + "]}";
  return s;
}

string branch::str(const size_t indent) {
  validate_branch(this);

  auto s = get_indent(indent) + "{ \"type\" : \"branch\", \"id\" : " + to_string(id) + ", \"bounding-box\" : " + bounding_box.str() + ", \"children\" : [ \n";

  const bool leaves = dynamic_cast<leaf*>(children[0].get()) != nullptr; // debug
  for(const auto& child : children) {
    const bool childIsLeaf = dynamic_cast<leaf*>(child.get()) != nullptr; // debug
    if(leaves != childIsLeaf)
      int debug = 42;

    s += child->str(indent + 1) + ", \n";
  }
  s.erase(s.end() - 3); // remove last ', \n'
  s += get_indent(indent) + "]}";

  validate_branch(this);

  return s;
}

} // namespace rtree
