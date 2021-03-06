/**
 * @file	Index.hpp 
 * @author	Jesús Carabaño Bravo <jcaraban@abo.fi>
 *
 * Node representing the raster cells index for one specific dimension
 */

#ifndef MAP_RUNTIME_DAG_INDEX_HPP_
#define MAP_RUNTIME_DAG_INDEX_HPP_

#include "Node.hpp"


namespace map { namespace detail {

struct Index : public Node
{
	// Internal declarations
	struct Key {
		Key(Index *node);
		bool operator==(const Key& k) const;
		NumDim dim;
	};
	struct Hash {
		std::size_t operator()(const Key& k) const;
	};

	// Factory
	static Node* Factory(DataSize ds, NumDim dim, MemOrder mo, BlockSize bs);
	static Node* Factory(Node *arg, NumDim dim);

	// Constructors & methods
	Index(const MetaData &meta, NumDim dim);
	void accept(Visitor *visitor);
	std::string getName() const;
	std::string signature() const;
	char classSignature() const;

	// Variables
	NumDim dim;
};

} } // namespace map::detail

#endif
