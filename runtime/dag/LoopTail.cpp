/**
 * @file	LoopTail.cpp 
 * @author	Jesús Carabaño Bravo <jcaraban@abo.fi>
 */

#include "LoopTail.hpp"
#include "LoopCond.hpp"
#include "../Runtime.hpp"
#include "../visitor/Visitor.hpp"
#include <functional>


namespace map { namespace detail {

// Internal declarations

LoopTail::Key::Key(LoopTail *node) {
	prev = node->prev();
	loop = node->loop();
}

bool LoopTail::Key::operator==(const Key& k) const {
	return (prev==k.prev && loop==k.loop);
}

std::size_t LoopTail::Hash::operator()(const Key& k) const {
	return std::hash<Node*>()(k.prev) ^ std::hash<LoopCond*>()(k.loop);
}

// Factory

//Node* LoopTail::Factory(LoopCond *loop, Node *prev) {
Node* LoopTail::Factory(Node *prev) {
	MetaData meta = prev->metadata();
	return new LoopTail(meta,prev);
}

Node* LoopTail::clone(std::unordered_map<Node*,Node*> other_to_this) {
	return new LoopTail(this,other_to_this);
}

// Constructors

//LoopTail::LoopTail(const MetaData &meta, LoopCond *loop, Node *prev)
LoopTail::LoopTail(const MetaData &meta, Node *prev)
	: Node(meta)
{
	owner_loop = nullptr; // 'head' knows who its 'loop' is
	twin_head = nullptr; // 'head' might have a twin 'tail'

	prev_list.reserve(1);
	this->addPrev(prev);

	prev->addNext(this); // 'prev' is a 'switch' that points to 'tail'
}

LoopTail::LoopTail(const LoopTail *other, std::unordered_map<Node*,Node*> other_to_this)
	: Node(other,other_to_this)
{
	Node *this_loop = other_to_this.find(other->owner_loop)->second;
	this->owner_loop = dynamic_cast<LoopCond*>(this_loop);
	
	/// Pushes itself into 'loop', because 'tail' did not live when 'loop' was created
	this->owner_loop->tail_list.push_back(this);
}

LoopTail::~LoopTail() {
	// Notifies its 'loop' about the deletion
	remove_value(this,owner_loop->tail_list);
}

// Methods

void LoopTail::accept(Visitor *visitor) {
	visitor->visit(this);
}

std::string LoopTail::getName() const {
	return "LoopTail";
}

std::string LoopTail::signature() const {
	assert(0);
	return "";
}

LoopCond* LoopTail::loop() const {
	return owner_loop;
}

Node* LoopTail::prev() const {
	return prev_list[1];
}

} } // namespace map::detail
