/**
 * @file	Feedback.hpp 
 * @author	Jesús Carabaño Bravo <jcaraban@abo.fi>
 *
 */

#ifndef MAP_RUNTIME_DAG_FEEDBACK_HPP_
#define MAP_RUNTIME_DAG_FEEDBACK_HPP_

#include "Node.hpp"


namespace map { namespace detail {

struct LoopCond; //!< Forward declaration
struct LoopHead; //!< Forward declaration

struct Feedback : public Node
{
	// Internal declarations
	struct Key {
		Key(Feedback *node);
		bool operator==(const Key& k) const;
		Node *prev;
		LoopCond *loop;
	};
	struct Hash {
		std::size_t operator()(const Key& k) const;
	};

	// Factory
	Node* clone(std::unordered_map<Node*,Node*> other_to_this);
	
	// Constructors
	Feedback(LoopCond *loop, LoopHead *prev);
	Feedback(LoopCond *loop, Feedback *feed_in, Node *prev);
	Feedback(const Feedback *other, std::unordered_map<Node*,Node*> other_to_this);

	// Methods
	void accept(Visitor *visitor);
	std::string getName() const;
	std::string signature() const;
	char classSignature() const;
	LoopCond* loop() const;
	Node* prev() const;
	bool feedIn() const;
	bool feedOut() const;
	Pattern pattern() const; // HEAD or TAIL

	//const NodeList& prevList() const;
	//const NodeList& nextList() const;

	// Variables
	LoopCond *owner_loop;
	bool in_or_out; //!< true / false --> FeedIn / FeedOut type
	Feedback *twin; // linked feedback (i.e. feed_in <--> feed_out)
};

} } // namespace map::detail

#endif
