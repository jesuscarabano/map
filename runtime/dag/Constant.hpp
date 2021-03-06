/**
 * @file	Constant.hpp 
 * @author	Jesús Carabaño Bravo <jcaraban@abo.fi>
 *
 * Node representing a Constant value (e.g. 0, 1.0f, 3.1415926535)
 */

#ifndef MAP_RUNTIME_DAG_CONSTANT_HPP_
#define MAP_RUNTIME_DAG_CONSTANT_HPP_

#include "Node.hpp"
 

namespace map { namespace detail {

struct Constant : public Node
{
	// Internal declarations
	struct Key {
		Key(Constant *node);
		bool operator==(const Key& k) const;
		NumDim num_dim;
		VariantType cnst;
	};
	struct Hash {
		std::size_t operator()(const Key& k) const;
	};

	// Factory
	static Node* Factory(VariantType arg, DataSize ds, DataType dt, MemOrder mo, BlockSize bs);

	// Constructors & methods
	Constant(const MetaData &meta, VariantType val);
	void accept(Visitor *visitor);
	std::string getName() const;
	std::string signature() const;
	char classSignature() const;

	// Variables
	VariantType cnst;
};

} } // namespace map::detail

#endif
