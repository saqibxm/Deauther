#ifndef LINKEDLIST_H_INCLUDED
#define LINKEDLIST_H_INCLUDED

#include <utility>

template <typename> class LinkedList;
template <typename> class ListIterator;

template <typename T>
bool operator==(const ListIterator<T> &rhs, const ListIterator<T> &lhs);

template <typename T>
struct Node
{
    friend class LinkedList<T>;
    using element_type = T;

    element_type data;
    mutable Node *next;

    Node() : data(), next(nullptr) { }
    Node(const T &val) : data(val), next(nullptr) { }
    template <typename...Args>
    Node(Args&&...args) noexcept
        : data(std::forward<Args>(args)...), next(nullptr) { }
    ~Node() { delete next; }

    // operator T() { return data; }
    Node& operator=(const element_type &val) { data = val; }
};

template <typename ListType>
struct ListIterator
{
    using Node = typename LinkedList<ListType>::node_type;
    friend class LinkedList<ListType>;
    friend bool operator==<ListType>(const ListIterator<ListType>&, const ListIterator<ListType>&);

public:
    // using iterator_category = std::forward_iterator_tag;
    ListIterator(Node *const node) : ptr(node) { }

    typename Node::element_type& operator*() { return ptr->data; }
    typename Node::element_type* operator->() { return &(ptr->data); }

    ListIterator& operator++() {
        if(ptr) ptr = ptr->next;
        return *this;
    }
    ListIterator operator++(int) {
        ListIterator ret = *this;
        ++*this;
        return ret;
    }

private:
    Node *ptr;
};

template <typename T> using SearchResult = std::pair<bool, Node<T>*>;

template <typename T>
class LinkedList
{
    friend class ListIterator<T>;

public:
    using node_type = Node<T>;
    using size_type = std::uint16_t;
    using iterator = ListIterator<T>;

    LinkedList() = default;
    LinkedList(const LinkedList&) = delete;


    template <typename...Args>
    T& emplace(Args&&...args);

    void insert(node_type *const pos, const T &val);
    node_type* insert(const T &val);
    void erase(node_type *node);

    void assign(node_type *const node, const T &val) const;
    node_type& find(T val) const;

    iterator begin() const;
    iterator end() const;

    size_type size() const noexcept { return count; }

    void traverse() const;
    void clear();

private:
    node_type *root = nullptr;
    size_type count = 0;
    SearchResult<T> nodeExists(const node_type *const sought) const;
};

template <typename T>
template <typename...Args>
T& LinkedList<T>::emplace(Args&&...args)
{
    node_type **insertion = &root;
    while(*insertion) {
        *insertion = (*insertion)->next;
    }
    *insertion = new node_type(std::forward<Args>(args)...);
    ++count;
    return (*insertion)->data;
}

template <typename T>
void LinkedList<T>::insert(node_type *const pos, const T &val)
{
    node_type *node = root;
    while(node != pos && node)
        node = node->next;

    if(!node)
        return;

    auto temp = node->next;
    node->next = new node_type(val);
    node->next->next = temp;
    ++count;
}

template <typename T>
auto LinkedList<T>::insert(const T &val) -> typename LinkedList<T>::node_type*
{
    if(root == nullptr)
    {
        root = new node_type(val);
        ++count;
        return root;
    }
    node_type *traversalNode = this->root;
    while(traversalNode->next != nullptr)
        traversalNode = traversalNode->next;

    return traversalNode->next = new node_type(val);
}

template <typename T>
void LinkedList<T>::erase(node_type *node)
{
    node_type *previousToTarget = root;
    node_type *nextToTarget = node->next;

    while(previousToTarget->next != node or previousToTarget->next != nullptr)
        previousToTarget = previousToTarget->next;

    delete previousToTarget->next;
    previousToTarget->next = nextToTarget;
    --count;
}

template <typename T>
void LinkedList<T>::clear()
{
    delete root;
    root = nullptr;
    count = 0;
}

template <typename T>
void LinkedList<T>::assign(node_type *const node, const T &val) const
{
    auto res = nodeExists(node);
    if(res.first)
        res.second->data = val;
}

template <typename T>
typename LinkedList<T>::node_type& LinkedList<T>::find(T val) const
{
    node_type *found = root;
    while(found)
    {
        if(val == found->data) break;
        found = found->next;
    }
    return *found;
}

template <typename T>
auto LinkedList<T>::begin() const -> typename LinkedList<T>::iterator
{
    return ListIterator<T>(root);
}

template <typename T>
auto LinkedList<T>::end() const -> typename LinkedList<T>::iterator
{
    iterator ret = begin();
    while(ret.ptr) ++ret;

    return ret;
}


template <typename T>
void LinkedList<T>::traverse() const
{
    if(!root) return;
    node_type *node = root;
    do
        node = node->next;
    while(node);
}


template <typename T>
SearchResult<T> LinkedList<T>::nodeExists(const node_type *const sought) const
{
    node_type *traversalNode = root;
    while(traversalNode)
    {
        if(traversalNode == sought)
            return std::make_pair(true, traversalNode);
        traversalNode = traversalNode->next;
    }

    return {false, nullptr};
}

template <typename T>
bool operator==(const ListIterator<T> &rhs, const ListIterator<T> &lhs)
{
    return rhs.ptr == lhs.ptr;
}

template <typename T>
bool operator!=(const ListIterator<T> &rhs, const ListIterator<T> &lhs)
{
    return !(rhs == lhs);
}

#endif // LINKEDLIST_H_INCLUDED