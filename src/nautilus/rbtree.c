/*
  Red Black Trees
  (C) 1999  Andrea Arcangeli <andrea@suse.de>
  (C) 2002  David Woodhouse <dwmw2@infradead.org>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <nautilus/rbtree.h>
#include <nautilus/nautilus.h>


static void __rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = rb_parent(node);

    if ((node->rb_right = right->rb_left))
	rb_set_parent(right->rb_left, node);
    right->rb_left = node;

    rb_set_parent(right, parent);

    if (parent)
	{
	    if (node == parent->rb_left)
		parent->rb_left = right;
	    else
		parent->rb_right = right;
	}
    else
	root->rb_node = right;
    rb_set_parent(node, right);
}

static void __rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
    struct rb_node * left = node->rb_left;
    struct rb_node * parent = rb_parent(node);

    if ((node->rb_left = left->rb_right)) {
	rb_set_parent(left->rb_right, node);
    }

    left->rb_right = node;

    rb_set_parent(left, parent);

    if (parent) {
	if (node == parent->rb_right) {
	    parent->rb_right = left;
	} else { 
	    parent->rb_left = left;
	}
    } else {
	root->rb_node = left;
    }

    rb_set_parent(node, left);
}

void nk_rb_insert_color(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *parent, *gparent;

    while ((parent = rb_parent(node)) && rb_is_red(parent))
	{
	    gparent = rb_parent(parent);

	    if (parent == gparent->rb_left)
		{
		    {
			register struct rb_node *uncle = gparent->rb_right;
			if (uncle && rb_is_red(uncle))
			    {
				rb_set_black(uncle);
				rb_set_black(parent);
				rb_set_red(gparent);
				node = gparent;
				continue;
			    }
		    }

		    if (parent->rb_right == node)
			{
			    register struct rb_node *tmp;
			    __rb_rotate_left(parent, root);
			    tmp = parent;
			    parent = node;
			    node = tmp;
			}

		    rb_set_black(parent);
		    rb_set_red(gparent);
		    __rb_rotate_right(gparent, root);
		} else {
		{
		    register struct rb_node *uncle = gparent->rb_left;
		    if (uncle && rb_is_red(uncle))
			{
			    rb_set_black(uncle);
			    rb_set_black(parent);
			    rb_set_red(gparent);
			    node = gparent;
			    continue;
			}
		}

		if (parent->rb_left == node)
		    {
			register struct rb_node *tmp;
			__rb_rotate_right(parent, root);
			tmp = parent;
			parent = node;
			node = tmp;
		    }

		rb_set_black(parent);
		rb_set_red(gparent);
		__rb_rotate_left(gparent, root);
	    }
	}

    rb_set_black(root->rb_node);
}


static void __rb_erase_color(struct rb_node *node, struct rb_node *parent,
			     struct rb_root *root)
{
    struct rb_node *other;

    while ((!node || rb_is_black(node)) && node != root->rb_node)
	{
	    if (parent->rb_left == node)
		{
		    other = parent->rb_right;
		    if (rb_is_red(other))
			{
			    rb_set_black(other);
			    rb_set_red(parent);
			    __rb_rotate_left(parent, root);
			    other = parent->rb_right;
			}
		    if ((!other->rb_left || rb_is_black(other->rb_left)) &&
			(!other->rb_right || rb_is_black(other->rb_right)))
			{
			    rb_set_red(other);
			    node = parent;
			    parent = rb_parent(node);
			}
		    else
			{
			    if (!other->rb_right || rb_is_black(other->rb_right))
				{
				    struct rb_node *o_left;
				    if ((o_left = other->rb_left))
					rb_set_black(o_left);
				    rb_set_red(other);
				    __rb_rotate_right(other, root);
				    other = parent->rb_right;
				}
			    rb_set_color(other, rb_color(parent));
			    rb_set_black(parent);
			    if (other->rb_right)
				rb_set_black(other->rb_right);
			    __rb_rotate_left(parent, root);
			    node = root->rb_node;
			    break;
			}
		}
	    else
		{
		    other = parent->rb_left;
		    if (rb_is_red(other))
			{
			    rb_set_black(other);
			    rb_set_red(parent);
			    __rb_rotate_right(parent, root);
			    other = parent->rb_left;
			}
		    if ((!other->rb_left || rb_is_black(other->rb_left)) &&
			(!other->rb_right || rb_is_black(other->rb_right)))
			{
			    rb_set_red(other);
			    node = parent;
			    parent = rb_parent(node);
			}
		    else
			{
			    if (!other->rb_left || rb_is_black(other->rb_left))
				{
				    register struct rb_node *o_right;
				    if ((o_right = other->rb_right))
					rb_set_black(o_right);
				    rb_set_red(other);
				    __rb_rotate_left(other, root);
				    other = parent->rb_left;
				}
			    rb_set_color(other, rb_color(parent));
			    rb_set_black(parent);
			    if (other->rb_left)
				rb_set_black(other->rb_left);
			    __rb_rotate_right(parent, root);
			    node = root->rb_node;
			    break;
			}
		}
	}
    if (node)
	rb_set_black(node);
}

void nk_rb_erase(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *child, *parent;
    int color;

    if (!node->rb_left)
	child = node->rb_right;
    else if (!node->rb_right)
	child = node->rb_left;
    else
	{
	    struct rb_node *old = node, *left;

	    node = node->rb_right;
	    while ((left = node->rb_left) != NULL)
		node = left;
	    child = node->rb_right;
	    parent = rb_parent(node);
	    color = rb_color(node);

	    if (child)
		rb_set_parent(child, parent);
	    if (parent == old) {
		parent->rb_right = child;
		parent = node;
	    } else
		parent->rb_left = child;

	    node->rb_parent_color = old->rb_parent_color;
	    node->rb_right = old->rb_right;
	    node->rb_left = old->rb_left;

	    if (rb_parent(old))
		{
		    if (rb_parent(old)->rb_left == old)
			rb_parent(old)->rb_left = node;
		    else
			rb_parent(old)->rb_right = node;
		} else
		root->rb_node = node;

	    rb_set_parent(old->rb_left, node);
	    if (old->rb_right)
		rb_set_parent(old->rb_right, node);
	    goto color;
	}

    parent = rb_parent(node);
    color = rb_color(node);

    if (child)
	rb_set_parent(child, parent);
    if (parent)
	{
	    if (parent->rb_left == node)
		parent->rb_left = child;
	    else
		parent->rb_right = child;
	}
    else
	root->rb_node = child;

 color:
    if (color == RB_BLACK && parent)
	__rb_erase_color(child, parent, root);
}


/*
 * This function returns the first node (in sort order) of the tree.
 */
struct rb_node *nk_rb_first(struct rb_root *root)
{
    struct rb_node	*n;

    n = root->rb_node;
    if (!n)
	return NULL;
    while (n->rb_left)
	n = n->rb_left;
    return n;
}


struct rb_node *nk_rb_last(struct rb_root *root)
{
    struct rb_node	*n;

    n = root->rb_node;
    if (!n)
	return NULL;
    while (n->rb_right)
	n = n->rb_right;
    return n;
}


struct rb_node *nk_rb_next(struct rb_node *node)
{
    struct rb_node *parent;

    /* If we have a right-hand child, go down and then left as far
       as we can. */
    if (node->rb_right) {
	node = node->rb_right; 
	while (node->rb_left)
	    node=node->rb_left;
	return node;
    }

    /* No right-hand children.  Everything down and left is
       smaller than us, so any 'next' node must be in the general
       direction of our parent. Go up the tree; any time the
       ancestor is a right-hand child of its parent, keep going
       up. First time it's a left-hand child of its parent, said
       parent is our 'next' node. */
    while ((parent = rb_parent(node)) && node == parent->rb_right)
	node = parent;

    return parent;
}


struct rb_node *nk_rb_prev(struct rb_node *node)
{
    struct rb_node *parent;

    /* If we have a left-hand child, go down and then right as far
       as we can. */
    if (node->rb_left) {
	node = node->rb_left; 
	while (node->rb_right)
	    node=node->rb_right;
	return node;
    }

    /* No left-hand children. Go up till we find an ancestor which
       is a right-hand child of its parent */
    while ((parent = rb_parent(node)) && node == parent->rb_left)
	node = parent;

    return parent;
}


void nk_rb_replace_node(struct rb_node *victim, struct rb_node *new,
			struct rb_root *root)
{
    struct rb_node *parent = rb_parent(victim);

    /* Set the surrounding nodes to point to the replacement */
    if (parent) {
	if (victim == parent->rb_left)
	    parent->rb_left = new;
	else
	    parent->rb_right = new;
    } else {
	root->rb_node = new;
    }
    if (victim->rb_left)
	rb_set_parent(victim->rb_left, new);
    if (victim->rb_right)
	rb_set_parent(victim->rb_right, new);

    /* Copy the pointers/colour from the victim to the replacement */
    *new = *victim;
}

//// Implementation of Join and Split ////////

int nk_get_rb_black_height(struct rb_node *node)
{
    int blackheight = 0;
    while (node != NULL) {
        if (rb_is_black(node))
            blackheight++;
        node = node->rb_left;
    }
    return blackheight;
}

struct rb_node* nk_rb_min_value_node(struct rb_node *root) {

     struct rb_node *ptr = root;

    while (ptr->rb_left != NULL)
        ptr = ptr->rb_left;

    return ptr;
}

struct rb_node* nk_rb_max_value_node(struct rb_node *root) {
    struct rb_node *ptr = root;

    while (ptr->rb_right != NULL)
        ptr = ptr->rb_right;

    return ptr;
}


struct rb_node* nk_rb_join_right(struct rb_node* tree1, struct rb_node* node, struct rb_node* tree2) {
	int bh1 = nk_get_rb_black_height(tree1);
	int bh2 = nk_get_rb_black_height(tree2);

	if(bh1 == bh2) {
		struct rb_node* newnode;
	       	newnode	= (struct rb_node*) malloc(sizeof(struct rb_node));
		*newnode = *node;
		newnode->rb_left = tree1;
		newnode->rb_right = tree2;
		rb_set_parent(tree1,newnode);
		rb_set_parent(tree2,newnode);
		rb_set_red(newnode);
		return newnode;
	}
	else {
		struct rb_node* llchild = tree1->rb_left;
		struct rb_node* lrchild = tree1->rb_right;
		int lcolor = rb_color(tree1);
		struct rb_node* rnode = nk_rb_join_right(lrchild,node,tree2);
		//struct rb_node* newnode = (struct rb_node*)malloc(sizeof(struct rb_node));
		struct rb_node *newnode = tree1;
		newnode->rb_right = rnode;
		newnode->rb_left = llchild;
		//rb_set_parent(llchild,newnode);
		rb_set_parent(rnode,newnode);
		struct rb_root* newroot = (struct rb_root*)malloc(sizeof(struct rb_root));
		newroot->rb_node = newnode;
		if(rb_is_black(newnode) && rb_is_red(newnode->rb_right) && rb_is_red(newnode->rb_right->rb_right)) {
			rb_set_black(newnode->rb_right->rb_right);
			__rb_rotate_left(newnode,newroot);
			return newnode;
		}
		else
			return newnode;

	}

}

struct rb_node* nk_rb_join_left(struct rb_node* tree1, struct rb_node* node, struct rb_node* tree2) {
	int bh1 = nk_get_rb_black_height(tree1);
	int bh2 = nk_get_rb_black_height(tree2);

	if(bh1 == bh2) {
		struct rb_node* newnode = (struct rb_node*)malloc(sizeof(struct rb_node));
		*newnode = *node;
		newnode->rb_left = tree1;
		newnode->rb_right = tree2;
		rb_set_parent(tree1,newnode);
		rb_set_parent(tree2,newnode);
		rb_set_red(newnode);
		return newnode;
	}
	else {
		struct rb_node* rlchild = tree2->rb_left;
		struct rb_node* rrchild = tree2->rb_right;
		int rcolor = rb_color(tree2);
		struct rb_node* lnode = nk_rb_join_left(rlchild,node,tree1);
		//struct rb_node* newnode = (struct rb_node*)malloc(sizeof(struct rb_node));
		struct rb_node *newnode = tree2;
		newnode->rb_left = lnode;
		newnode->rb_right = rrchild;
		rb_set_parent(rrchild,newnode);
		rb_set_parent(lnode,newnode);
		struct rb_root* newroot = (struct rb_root*)malloc(sizeof(struct rb_root));
		newroot->rb_node = newnode;
		if(rb_is_black(newnode) && rb_is_red(newnode->rb_left) && rb_is_red(newnode->rb_left->rb_left)) {
			rb_set_black(newnode->rb_left->rb_left);
			__rb_rotate_right(newnode,newroot);
			return newnode;
		}
		else
			return newnode;

	}

}

struct rb_node* nk_rb_join(struct rb_node* tree1, struct rb_node* node, struct rb_node* tree2) {

	int bh1 = nk_get_rb_black_height(tree1);
	int bh2 = nk_get_rb_black_height(tree2);
	if(bh1 > bh2) {
		struct rb_node* newnode = nk_rb_join_right(tree1,node,tree2);
		if(rb_is_red(newnode) && rb_is_red(newnode->rb_right)) {
			rb_set_black(newnode);
		}
		return newnode;
	}
	else if(bh1<bh2) {
		struct rb_node* newnode = nk_rb_join_left(tree1,node,tree2);
		if(rb_is_red(newnode) && rb_is_red(newnode->rb_left)) {
			rb_set_black(newnode);
		}
		return newnode;
	}
	else if(rb_is_black(tree1) && rb_is_black(tree2)) {
		struct rb_node* newnode = (struct rb_node*)malloc(sizeof(struct rb_node));
		*newnode = *node;
		rb_set_parent(tree1,newnode);
		rb_set_parent(tree2,newnode);
		newnode->rb_left = tree1;
		newnode->rb_right = tree2;
		rb_set_red(newnode);
		return newnode;
	}
	else {
		struct rb_node* newnode = (struct rb_node*)malloc(sizeof(struct rb_node));
		rb_set_parent(tree1,newnode);
		rb_set_parent(tree2,newnode);
		*newnode = *node;
		newnode->rb_left = tree1;
		newnode->rb_right = tree2;
		rb_set_black(newnode);
		return newnode;

	}

}


struct rb_split_tree* split(struct rb_node* T, struct rb_node* node) {
	if(T == NULL) return NULL;
	struct rb_node* left = T->rb_left;
	struct rb_node* right = T->rb_right;
	if(T==node) {
		struct rb_split_tree* result = (struct rb_split_tree*)malloc(sizeof(struct rb_split_tree));
		result->left_tree = left;
		result->right_tree = right;
		result->node = node;
		return result;

	}
	else if(node<T) {
		struct rb_split_tree* temp = split(left,node);
		struct rb_node* joined = nk_rb_join(temp->right_tree,T,right);
		struct rb_split_tree* result = (struct rb_split_tree*)malloc(sizeof(struct rb_split_tree));
		result->left_tree = temp->left_tree;
		result->right_tree = joined;
		result->node = temp->node;
		return result;
	}
	else if(node>T) {
		struct rb_split_tree* temp = split(right,node);
		struct rb_node* joined = nk_rb_join(temp->left_tree,T,left);
		struct rb_split_tree* result = (struct rb_split_tree*)malloc(sizeof(struct rb_split_tree));
		result->left_tree = joined;
		result->right_tree = temp->right_tree;
		result->node = temp->node;
		return result;
	}

	return NULL;



}


