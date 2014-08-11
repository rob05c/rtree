#ifndef rtreeH
#define rtreeH

#include <vector>
#include <memory>
#include <limits>

namespace rtree {

/// max node members. After which, a split occurs
const size_t max_fill = 3;

typedef double ord_t; // ordinate type. Not a coordinate, because there's only one.
const ord_t ord_t_max = std::numeric_limits<double>::max();
const ord_t ord_t_min = std::numeric_limits<double>::lowest();

struct point {
  ord_t x;
  ord_t y;
// ord_t z;
};

struct rect {
  ord_t top;
  ord_t left;
  ord_t bottom;
  ord_t right;

  std::string str();
};

struct data {
  rect r;
  int  key;

  std::string str();
};

struct branch;
struct tree;

struct node {
  static size_t next_id;

  node(tree* rtree_, branch* parent_)
    : rtree(rtree_)
    , parent(parent_)
    , id(next_id++)
  {}

  virtual bool insert(const data& d) = 0;
  virtual void calculate_bounding_box() = 0; ///< should be called after children are changed or populated
  virtual std::string str(const size_t indent) = 0;

  void reroot();
  ord_t expansion(const rect& r); ///< get how much it will be necessary to expand this rect, if we insert r

  rect bounding_box;
  tree* rtree; ///< \todo remove. This exists for updating the root node
  branch* parent;
  size_t id;
private:
  node();
};

struct branch : public node {
  branch(tree* rtree_, branch* parent_) : node(rtree_, parent_) {}

  virtual void calculate_bounding_box();
  virtual bool insert(const data& d);
  virtual std::string str(const size_t indent);

  void add(std::unique_ptr<node>&);
  void split(std::unique_ptr<node>& n);
  std::vector<std::unique_ptr<node>>::iterator smallest_expansion_child(const rect& r);

  std::vector<std::unique_ptr<node>> children;
private:
  branch();
};

struct leaf : public node {
  leaf(tree* rtree_, branch* parent_) : node(rtree_, parent_) {}

  virtual void calculate_bounding_box();
  virtual bool insert(const data& d);
  virtual std::string str(const size_t indent);
  
  void split(const data& d);
  std::vector<std::unique_ptr<data>> children;
private:
  leaf();
};



class tree {
public:
  tree() {}
  virtual ~tree() {}
  
  std::string str();

  void insert(const data& d);
  std::unique_ptr<node> root; ///< \todo make protected and friend branch/leaf
};

} // namespace rtree

#endif
