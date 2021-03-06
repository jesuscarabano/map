/**
 * @file	Fusioner.cpp 
 * @author	Jesús Carabaño Bravo <jcaraban@abo.fi>
 *
 * Note: pipe-gently is not lossless, it could hide the optimal fusion when considering the data type (e.g. B8)
 * Note: pipeFusing groups does not accurately reflect the pattern than one group sees of another (notebook)
 * Note: sorting has to go after linking or will break Radiating (out cl_mem arguments are moved if sorted)
 *
 * TODO: what about going bottom-up in inversed id order?
 */

#include "Fusioner.hpp"
#include "../Runtime.hpp"
#include <algorithm>
#include <map>


namespace map { namespace detail {

/***********
   Methods
 ***********/

Fusioner::Fusioner(OwnerGroupList& group_list)
	: group_list(group_list)
{ }

void Fusioner::clear() {
	visited.clear(); // @
	group_list.clear();
	group_list_of.clear();
}

void Fusioner::fuse(NodeList list) {
	TimedRegion region(Runtime::getClock(),FUSION);
	clear();

	// Goes down, creating groups and fusing nodes

	for (auto node : list) { // Creates groups and pipe-fuses gently ## 1st fusion stage ##
		process(node);
		pipeGently(node);
	}

//print(); // @

	for (auto node : list) {// Flat-fuses gently ## still 1st stage ##
		flatGently(node);
	}

//print(); // @

	for (auto it=list.rbegin(); it!=list.rend(); it++) {
		assert(group_list_of[*it].size() == 1);
		processBU(group_list_of[*it].front()); // Goes up group by group # 2nd fusion stage ##
	}

//print(); // @

	auto free = [](Node *n){ return n->pattern()==FREE; };
	forwarding(free); // Replicates lonely free nodes ## 3rd fusion tage ##

//print(); // @

	linking(); // Marks as input/output all those nodes in the group boundary

	// fuse the output nodes only at the end

	sorting(); // Shorts 'group_list' (in topological order) and group_list[*]->node_list (in id order)

	print(); // Prints groups and nodes once linked & sorted
}

Group* Fusioner::newGroup() {
	return Runtime::getInstance().addGroup( new Group() );
}

void Fusioner::removeGroup(Group *group) {
	auto pred = [&](const std::unique_ptr<Group> &g) { return g.get() == group; };
	group_list.erase(std::remove_if(group_list.begin(),group_list.end(),pred),group_list.end());
}

bool Fusioner::canPipeFuse(Group *top, Group *bot) {
	if (top == bot)
		return false; // same node
	for (auto next : top->nextList())
		if (next != bot && next->isNext(bot))
			return false; // found cycle

	return detail::canPipeFuse(bot->prevPattern(top),top->nextPattern(bot))
	       && detail::canFlatFuse(top->pattern(),bot->pattern());
}

bool Fusioner::canFlatFuse(Group *left, Group *right) {
	if (left == right)
		return false; // same node
	if (left->isNext(right) || right->isNext(left))
		return false; // found cycle

	return detail::canFlatFuse(left->pattern(),right->pattern());
}

Group* Fusioner::pipeFuseGroup(Group *&top, Group *&bot) {
	assert(canPipeFuse(top,bot));
	
	// Moves 'bot' nodes to 'top'
	for (auto node : bot->nodeList()) {
		top->addNode(node);
		remove_value(bot,group_list_of[node]);
		group_list_of[node].push_back(top);
	}
	// Moves 'bot' input-nodes to 'top'
	for (auto node : bot->inputList()) {
		top->addInputNode(node);
		remove_value(bot,group_list_of[node]);
		group_list_of[node].push_back(top);
	}
	// Moves 'bot' output-nodes to 'top'
	for (auto node : bot->outputList()) {
		top->addOutputNode(node);
		remove_value(bot,group_list_of[node]);
		group_list_of[node].push_back(top);
	}
	
	// Moves 'bot' prev-groups to 'top'
	for (auto prev : bot->prevList()) {
		if (prev == top)
			continue;

		// Adds 'top' as next-group of 'prev'
		prev->addNext(top,prev->nextPattern(bot));

		// Adds 'prev' as prev-group of 'top'
		top->addPrev(prev,bot->prevPattern(prev));

		// 'prev' no longer points to 'bot'
		prev->removeNext(bot);
	}

	// Moves 'bot' next-groups to 'top'
	for (auto next : bot->nextList()) {
		assert(next != top); // Otherwise there was a cycle

		// Adds 'top' as prev-group of 'next', (accumulates both patterns)
		next->addPrev(top,next->prevPattern(bot)+bot->prevPattern(top));
		// @ next-to-top pattern is a worst-case

		// Adds 'next' as next-group of 'top'
		top->addNext(next,bot->nextPattern(next));

		// 'next' no longer points to 'bot'
		next->removePrev(bot);
	}

	// No need to touch 'top' next-groups

	// Updates 'top' prev-groups with 'bot' pattern
	for (auto prev : top->prevList()) {
		prev->addNext(top,top->nextPattern(bot));
		// @ prev-to-top pattern is a worst-case
	}

	// 'top' no longer points to 'bot'
	top->removeNext(bot);

	// Pipe-fuses 'top' to 'bot' pattern
	top->pattern() += bot->pattern();

	// Finally erases 'bot' from the main list of groups
	removeGroup(bot);

	Group *ret = top;
	top = nullptr;
	bot = nullptr;
	return ret;
}

Group* Fusioner::flatFuseGroup(Group *&left, Group *&right) {
	assert(canFlatFuse(left,right));
	
	// Moves 'right' nodes to 'left'
	for (auto node : right->nodeList()) {
		left->addNode(node);
		remove_value(right,group_list_of[node]);
		group_list_of[node].push_back(left);
	}
	// Moves 'right' input-nodes to 'left'
	for (auto node : right->inputList()) {
		left->addInputNode(node);
		remove_value(right,group_list_of[node]);
		group_list_of[node].push_back(left);
	}
	// Moves 'right' output-nodes to 'left'
	for (auto node : right->outputList()) {
		left->addOutputNode(node);
		remove_value(right,group_list_of[node]);
		group_list_of[node].push_back(left);
	}
	
	// Moves 'right' prev-groups to 'left'
	for (auto prev : right->prevList()) {
		assert(prev != left);

		// Adds 'left' as next-group of 'prev'
		prev->addNext(left,prev->nextPattern(right));

		// Adds 'prev' as prev-group of 'left'
		left->addPrev(prev,right->prevPattern(prev));

		// 'prev' no longer points to 'right'
		prev->removeNext(right);
	}

	// Moves 'right' next-groups to 'left'
	for (auto next : right->nextList()) {
		assert(next != left);

		// Adds 'left' as prev-group of 'next'
		next->addPrev(left,next->prevPattern(right));

		// Adds 'next' as next-group of 'left'
		left->addNext(next,right->nextPattern(next));

		// 'next' no longer points to 'right'
		next->removePrev(right);
	}

	// flat-fuses 'left' with 'right' pattern
	left->pattern() += right->pattern();

	// Finally erases 'right' from the main list of groups
	removeGroup(right);
	
	Group *ret = left;
	left = nullptr;
	right = nullptr;
	return ret;
}

void Fusioner::process(Node *node) {
	Group *new_group = newGroup(); // Creates a new group for the node
	new_group->addAutoNode(node); // Adds node to the new group
	group_list_of[node].push_back(new_group); // Adds the new group to the group list of node

	for (auto prev : node->prevList()) {
		Group *prev_group = group_list_of[prev].front(); // Nodes have max. 1 group at this point
		prev_group->addNext(new_group,new_group->pattern()); // Giving the pattern this way works because
		new_group->addPrev(prev_group,prev_group->pattern()); // only LOCAL / FREE patters are fused now
	}
}

void Fusioner::pipeGently(Node *node) {
	if (not Runtime::getConfig().code_fusion)
		return;
	auto isFreeOrLocal = [](Group *group) { return group->pattern().is(FREE) || group->pattern().is(LOCAL); };
	Group *new_group = group_list_of[node].front();

	int i = 0;
	while (i < new_group->prevList().size()) {
		Group *prev_group = new_group->prevList()[i];
		i++;
		bool fuse_free = isFreeOrLocal(new_group) && isFreeOrLocal(prev_group);
		bool fuse_dnd0 = not (new_group->numdim() != D0 && prev_group->numdim() == D0 && prev_group->pattern() != FREE);

		if (fuse_free && fuse_dnd0 && prev_group->nextList().size() == 1 && canPipeFuse(prev_group,new_group)) {
			new_group = pipeFuseGroup(prev_group,new_group);
			i = 0; // rather than resetting, could be improved with a queue
		}
	}
}

void Fusioner::flatGently(Node *node) {
	if (not Runtime::getConfig().code_fusion)
		return;
	auto isFreeOrLocal = [](Group *group) { return group->pattern().is(FREE) || group->pattern().is(LOCAL); };
	Group *node_group = group_list_of[node].front();
	
	if (node_group->nextList().size() < 2)
		return; // Nothing to flat-fuse here

	int i = 0;
	while (i < node->nextList().size())
	{
		Node *left = node->nextList()[i++];
		Group *left_group = group_list_of[left].front();
		if (not isFreeOrLocal(left_group) || left_group == node_group)
			continue;

		int j = i; // i was already incremented
		while (j < node->nextList().size())
		{
			Node *right = node->nextList()[j++];
			Group *right_group = group_list_of[right].front();
			if (not isFreeOrLocal(right_group) || right_group == node_group || right_group == left_group)
				continue;

			if (canFlatFuse(left_group,right_group)) {
				left_group = flatFuseGroup(left_group,right_group);
			}
		}
	}
}

void Fusioner::process(Group *group) {
	if (!Runtime::getConfig().code_fusion)
		return;

	//// Flat-fusion
	int i = 0;
	while (i < group->nextList().size())
	{
		int j = i+1;
		while (j < group->nextList().size())
		{
			Group *left = group->nextList()[i];
			Group *right = group->nextList()[j];
			// @
			bool common_input = false;
			for (auto node : group->nodeList())
				for (auto next : node->nextList())
					if (is_included(next,left->nodeList()))
						for (auto next : node->nextList())
							if (is_included(next,right->nodeList()))
								common_input = true;
			//
			if (common_input && canFlatFuse(left,right)) {
				left = flatFuseGroup(left,right);
				i = j = 0; // reset
			} else {
				j++;
			}
		}
		i++;
	}
}

void Fusioner::processBU(Group *group) { // @
	if (!Runtime::getConfig().code_fusion)
		return;
	
	if (visited.find(group) != visited.end())
		return;
	visited.insert(group);

	//// Pipe-fusion
	int i = 0;
	while (i < group->prevList().size())
	{
		Group *bot = group;
		Group *top = group->prevList()[i++];
		bool d0dn = not (top->pattern() != FREE && top->numdim() == D0 && bot->numdim() != D0);

		if (d0dn && canPipeFuse(top,bot)) {
			group = pipeFuseGroup(top,bot);
			i = 0; // reset
		}
	}

	//// Going up
	int s = group->prevList().size();
	i = 0;
	while (i < s) { 
		processBU(group->prevList()[i++]);
		if (s != group->prevList().size()) {
			s = group->prevList().size();
			i = 0;
		}
	}
/*
	//// Flat-fusion
	i = 0;
	while (i < group->nextList().size())
	{
		int j = i+1;
		while (j < group->nextList().size())
		{
			Group *left = group->nextList()[i];
			Group *right = group->nextList()[j];
			if (canFlatFuse(left,right)) {
				left = flatFuseGroup(left,right);
				i = j = 0; // reset
			} else {
				j++;
			}
		}
		i++;
	}
*/
}

void Fusioner::forwarding(std::function<bool(Node*)> for_pred) {
	std::map<std::pair<Group*,Group*>,std::vector<Node*>> forward;
	auto all_nodes = [](Group *group){
		return full_join(group->inputList(),full_join(group->nodeList(),group->outputList()));
	};

	// Node forwarding phase

	for (auto &i : group_list) { // For every 'group' in group_list...
		Group *group = i.get();
		//assert(!group->nodeList().empty());

		for (auto &node : all_nodes(group)) { // For every 'node' in 'group'
			if (!for_pred(node))
				continue; // Which satisfy the given predicate
			bool forwarded = false;

			for (auto next_node : node->nextList()) { // For every 'next-node' of 'node'
				for (auto next_group : group_list_of[next_node]) { // For every group (aka 'next-group') of 'next-node'
					if (!is_included(node,all_nodes(next_group))) // If 'node' is not included in 'next-group'
					{
						next_group->addAutoNode(node); // Forward 'node' into 'next-group'
						group_list_of[node].push_back(next_group); // Adds 'next-group' into 'group_list' of 'node'
						forwarded = true;
						forward[{group,next_group}].push_back(node);
					}
				}
			}

			if (forwarded) // If 'node' was forwarded
			{
				auto pred = [&](Node *n) { return !is_included(n,group->nodeList()); };
				if (std::all_of(node->nextList().begin(),node->nextList().end(),pred)) {
					group->removeAutoNode(node); // If all nexts of 'node' are outside 'group', removes 'node'
					remove_value(group,group_list_of[node]); // Removes 'group' from 'group_list' of 'node'
				}
			}
		}
	}

	// Group-to-group unlinking phase

	for (auto f : forward) { // For every link 'group'<->'next-group'
		Group *group = f.first.first, *next_group = f.first.second;
		bool linked = false;
		for (auto node : left_join(group->nodeList(),f.second))
			if (node->pattern() != FREE)
				for (auto next_node : node->nextList())
					if (is_included(next_node,next_group->nodeList()))
						linked = true;
		if (!linked) { // If there remains no links between nodes, remove groups link
			group->removeNext(next_group);
			next_group->removePrev(group);
		}
	}

	// Group removing phase
	auto rm_pred = [&](const std::unique_ptr<Group> &g) { return all_nodes(g.get()).empty(); };
	group_list.erase(std::remove_if(group_list.begin(),group_list.end(),rm_pred),group_list.end());
}

void Fusioner::linking() {
	// For group, node, next-node, next-group: if node !€ in next-group, node becomes in/out-node

	for (auto &i : group_list) { // For every 'group' in group_list...
		Group *group = i.get();
		assert(!group->nodeList().empty());

		for (auto &node : group->nodeList()) { // For every 'node' in 'group'
			//assert(!node->nextList().empty());

			for (auto next_node : node->nextList()) { // For every 'next-node' of 'node'
				//assert(!next_node->groupList().empty() || next_node->isOutput());

				for (auto next_group : group_list_of[next_node]) { // For every group (aka 'next-group') of 'next-node'
					assert(!next_group->nodeList().empty());
					
					if (!is_included(node,next_group->nodeList())) { // If 'node' is not included in 'next-group'
						group->addOutputNode(node); // 'node' becomes an output of its 'group'
						next_group->addInputNode(node); // 'next_group's accept 'node' as an input
					}
				}
			}
		}
	}

	// Nodes withs patterns presenting intra-denepdencies (e.g. Radiating, Spreading) become output of their group

	for (auto &i : group_list) {  // For every Radiating / Spreading 'group' in group_list...
		Group *group = i.get();
		Pattern pat = group->pattern();
		if (!pat.is(RADIAL) && !pat.is(SPREAD))
			continue;

		for (auto &node : group->nodeList()) { // For every Radiating / Spreading 'node' in 'group'
			Pattern pat = node->pattern();
			if (!pat.is(RADIAL) && !pat.is(SPREAD))
				continue;

			group->addOutputNode(node); // 'node' becomes an output of its 'group'
		}
	}

	// Moves D0-FREE nodes from the node_list to the input_list, improves the reutilization rate of compiled kernels

	for (auto &g : group_list) { // For every 'group' in group_list...
		Group *group = g.get();
		int i = 0;
		while (i < group->nodeList().size()) { // For every 'node' in 'group'
			Node *node = group->nodeList()[i];
			if (node->numdim() == D0 && node->pattern() == FREE) { // Which is D0 and FREE
				group->removeNode(node); // Removes from nodeList()
				group->addInputNode(node); // Inserts into inputList()
			} else {
				i++;
			}
		}
	}
}

void Fusioner::sorting() {
	// For each group, sorts its lists of nodes by Node id
	for (auto &group : group_list) {
		std::sort(group->node_list.begin(),group->node_list.end(),node_id_less());
		std::sort(group->in_list.begin(),group->in_list.end(),node_id_less());
		std::sort(group->out_list.begin(),group->out_list.end(),node_id_less());
	}

	// Topological sort of group_list, in order of dependencies and last-node id
	auto less = [](const std::unique_ptr<Group> &a, const std::unique_ptr<Group> &b){
		return a->isNext(b.get()) ? true : a->isPrev(b.get()) ? false : a->nodeList().back()->id < b->nodeList().back()->id;
	};
	std::sort(group_list.begin(),group_list.end(),less);
	
	// Numerate groups with an 'id' after sorting
	for (auto &group : group_list) {
		group->id = Group::id_count++;
	}

	// @ It would be good to sort the group prev_list and next_list,
	// but prev_pat and next_pat must be ordered accordingly
	
	/*
	sorted_group_list.clear();
	visited.clear();
	for (auto &g : group_list)
		toposort(g.get());
	std::reverse(sorted_group_list.begin(),sorted_group_list.end());
	*/
	
	/*
	// Sorts the original 'group_list' by id
	auto cmp = [](const std::unique_ptr<Group> &a, const std::unique_ptr<Group> &b){ return a->id < b->id; };
	std::sort(group_list.begin(),group_list.end(),cmp);
	*/
}

void Fusioner::toposort(Group *group) {
	if (visited.find(group) != visited.end())
		return;
	visited.insert(group);

	for (auto next : group->nextList())
		toposort(next);

	sorted_group_list.push_back(group);
}

void Fusioner::print() {
	for (auto &i : group_list) {
		std::cout << i->pattern() << "  " << i.get() << "  " << std::endl;
		for (auto j : i->inputList())
			std::cout << "    " << j->getName() << " : " << j->id << std::endl;
		std::cout << "    --" << std::endl;
		for (auto j : i->nodeList())
			std::cout << "    " << j->getName() << " : " << j->id << std::endl;
		std::cout << "    --" << std::endl;
		for (auto j : i->outputList())
			std::cout << "    " << j->getName() << " : " << j->id << std::endl;
		std::cout << "  prev:" << std::endl;
		for (auto j=i->prevList().begin(); j!=i->prevList().end(); j++)
			std::cout << "    " << (*j) << " " << i->prevPattern(j) << std::endl;
		std::cout << "  next:" << std::endl;
		for (auto j=i->nextList().begin(); j!=i->nextList().end(); j++)
			std::cout << "    " << (*j) << " " << i->nextPattern(j) << std::endl;
		std::cout << std::endl;
	}
	std::cout << "--------------------" << std::endl;
}

} } // namespace map::detail
