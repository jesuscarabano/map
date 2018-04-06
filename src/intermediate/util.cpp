/**
 * @file	util.cpp 
 * @author	Jesús Carabaño Bravo <jcaraban@abo.fi>
 */

#include "util.hpp"


namespace map { namespace detail {

bool node_id_less::operator()(const Node *lhs, const Node *rhs) {
	return lhs->id < rhs->id;
}

bool node_id_equal::operator()(const Node *lhs, const Node *rhs) {
	return lhs->id == rhs->id;
}

bool node_id_greater::operator()(const Node *lhs, const Node *rhs) {
	return lhs->id > rhs->id;
}
/*
bool cluster_id_equal::operator()(const Cluster *lhs, const Cluster *rhs) {
	return lhs->id == rhs->id;
}

bool is_included(const Node *node, const NodeList &list) {
	auto pred = [&](Node *n) { return n == node; }; // @ address comparison
	return std::find_if(list.begin(),list.end(),pred) != list.end();
}

bool is_included(const Cluster *cluster, const ClusterList &list) {
	auto pred = [&](Cluster *g) { return g == cluster; }; // @ address comparison
	return std::find_if(list.begin(),list.end(),pred) != list.end();
}

NodeList inner_join(const NodeList &lhs, const NodeList &rhs) {
	NodeList join;
	for (auto left : lhs) for (auto right : rhs) // double for
		if (left == right) {
			join.push_back(left);
			break; // push & break 2nd for
		}
	return join;
}

NodeList full_join(const NodeList &lhs, const NodeList &rhs) {
	NodeList join;
	join.reserve(lhs.size()+rhs.size());
	join.insert(join.end(),lhs.begin(),lhs.end());
	join.insert(join.end(),rhs.begin(),rhs.end());
	return join;
}

NodeList full_unique_join(const NodeList &lhs, const NodeList &rhs) {
	NodeList join;
	join.reserve(lhs.size()+rhs.size());
	join.insert(join.end(),lhs.begin(),lhs.end());
	for (auto right : rhs)
		if (not is_included(right,join))
			join.push_back(right);
	return join;
}

NodeList left_join(const NodeList &lhs, const NodeList &rhs) {
	NodeList join;
	for (auto left : lhs)
		if (!is_included(left,rhs))
			join.push_back(left);
	return join;
}

void remove_value(const Node *node, NodeList &list) {
	list.erase(std::remove(list.begin(),list.end(),node),list.end());
}

void remove_value(const Cluster *cluster, ClusterList &list) {
	list.erase(std::remove(list.begin(),list.end(),cluster),list.end());
}

int value_position(const Node *node, const NodeList &list) {
	auto it = std::find(list.begin(),list.end(),node);
	assert(it != list.end());
	return std::distance(list.begin(),it);
}
*/
Pattern isInputOf(const Node *node, const Cluster *cluster) {
	Pattern pat;
	for (auto next : node->nextList())
		if (is_included(next,cluster->nodeList())) {
			pat += isInputOf(next,cluster);
			pat += next->pattern();
		}
	return pat;
}

} } // namespace map::detail