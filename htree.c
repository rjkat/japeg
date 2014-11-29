#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "htree.h"

#define HTREE_NUM_BRANCHES 2
#define HTREE_BRANCH_LEFT  0
#define HTREE_BRANCH_RIGHT 1

typedef struct hnode_s hnode;

struct hnode_s {
   int is_leaf;
   union {
      uint8_t code;
      hnode  *children[HTREE_NUM_BRANCHES];
   };
};

struct htree_s {
   size_t max_string_bits;
   size_t bytes_per_code;
   hnode *root;
};

static hnode *make_leaf_node(uint8_t code) {
   hnode *new = calloc(1, sizeof(hnode));
   assert(new);
   new->is_leaf = 1;
   new->code = code;
   return new;
}

static hnode *make_node(void) {
   hnode *new = calloc(1, sizeof(hnode));
   assert(new);
   return new;
}

static int htree_add_code(hnode *parent, size_t depth, size_t string_length, uint8_t code) {
   /* If we have hit the end of the tree, return */
   if (parent == NULL || parent->is_leaf) {
      return 1;
   }
   hnode **left  = &parent->children[HTREE_BRANCH_LEFT];
   hnode **right = &parent->children[HTREE_BRANCH_RIGHT];
   /* If we are at the right depth, try to add the code */
   if (string_length == depth) {
      /* No space to add code */
      if (*left && *right) {
         return 1;
      }
      hnode *node = make_leaf_node(code);
      /* Add on left first */
      if (!*left) {
         *left  = node;
      } else {
         *right = node;
      }
      /* Success */
      return 0;
   } else {
      /* We need to go deeper */
      if (!(*left)) {
         *left  = make_node();
      }
      if (!(*right)) {
         *right = make_node();
      }
      int error = htree_add_code(*left, depth + 1, string_length, code);
      if (error) {
         error = htree_add_code(*right, depth + 1, string_length, code);
      }
      return error;
   }
}

htree *htree_create(size_t max_string_bits, size_t code_bits, const size_t *num_codes, uint8_t **codes) {
   size_t string_length;
   int error = 0;
   htree *tree = malloc(sizeof(htree));
   assert(tree);
   tree->max_string_bits = max_string_bits;
   tree->bytes_per_code  = ((code_bits + 7) / 8);
   tree->root = make_node();
   assert(tree->root);
   for (string_length = 1; string_length <= max_string_bits && !error; string_length++) {
      size_t i;
      for (i = 0; i < num_codes[string_length - 1] && !error; i += tree->bytes_per_code) {
         error = htree_add_code(tree->root
                               ,1
                               ,string_length
                               ,codes[string_length - 1][i]);      
      }
   }
   return tree;
}

static void htree_destroy_internal(hnode *node) {
   if (node) {
      if (!node->is_leaf) {
         htree_destroy_internal(node->children[HTREE_BRANCH_LEFT]);
         htree_destroy_internal(node->children[HTREE_BRANCH_RIGHT]);
      }
      free(node);
   }
}

void htree_destroy(htree *tree) {
   if (tree) {
      htree_destroy_internal(tree->root);
      free(tree);
   }
}

int htree_get(const htree *tree, jpeg_stream *stream, uint8_t *code) {
   size_t   current_byte = 0;
   int      move;
   int      done = 0;
   int      failed = 1;
   size_t   bits_read;
   hnode   *current_node;
   assert(tree);
   bits_read = 0;
   current_node = tree->root;
   while (bits_read < tree->max_string_bits && !done) {
      /* If we haven't hit the end of the tree */
      if (!current_node) {
         done = 1;
      } else if (current_node->is_leaf) {
         *code  = current_node->code;
         done   = 1;
         failed = 0;
      } else {
         /* Grab the next bit of the bit string */
         move = jpeg_stream_get_next_bit(stream);
         /* Move down the tree based on its value
          * 0 => left
          * 1 => right 
          */
         current_node = current_node->children[move];
         bits_read += 1;
      }
      current_byte += 1;
   }
   /* Handle case where code is 16 bits long */
   if (current_node && current_node->is_leaf) {
      failed = 0;
      *code = current_node->code;
   }
   /* Return 0 if successful, 1 otherwise */
   return failed;
}
