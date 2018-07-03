#include "./huffman.h"

#include "./quicksort.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>




typedef struct sc_huffman_node sc_huffman_node_t;

struct sc_huffman_node {
	sc_ll_node_t *node;
	sc_huffman_node_t *previous;
	sc_huffman_node_t *next;
};




sc_result_t
sc_huffman_init(sc_huffman_t *const context) {
	if (context == NULL) {
		return SC_E_NULL;
	}

	memset(context, 0, sizeof(*context));

	return SC_E_SUCCESS;
}


sc_result_t
sc_huffman_clear(sc_huffman_t *const context) {
	if (context == NULL) {
		return SC_E_NULL;
	}

	sc_ll_node_free(context->tree_root, 1);

	memset(context, 0, sizeof(*context));

	return SC_E_SUCCESS;
}




sc_result_t
sc_huffman_process(sc_huffman_t *const restrict context, const void *const restrict data, const size_t size) {
	if (context == NULL || data == NULL) {
		return SC_E_NULL;
	}

	if (size == 0) {
		return SC_E_PARAM;
	}

	if (context->tree_root != NULL) {
		return SC_E_LOCKED;
	}


	const uint8_t *const data8 = ((const uint8_t *const)data);
	sc_qs_t *const freqs = context->frequencies;


	/* Use the value as an index to a counter registry, and for each value in the provided data increment its
	** frequency by as many times as it occurs. */

	register size_t i;
	for (i = size; i--;) {
		++freqs[data8[i]];
	}


	return 0;
}




sc_result_t
sc_huffman_tree_build(sc_huffman_t *const context) {
	if (context == NULL) {
		return SC_E_NULL;
	}

	if (context->tree_root != NULL) {
		return SC_E_LOCKED;
	}


	size_t n = 0;
	register unsigned int i;

	sc_qs_pair_t freqs_sorted[256], *fqp; {
		const sc_qs_t *const freqs = context->frequencies;

		/* Set the tag and sortable value of all pairs according to
 		** the measured frequencies. */
		for (i = 256; i--;) {
			freqs_sorted[i].tag = i;
			freqs_sorted[i].qsvalue = freqs[i];
		}

		sc_quicksort(freqs_sorted, 0, 256, SC_QS_MODE_ASCENDING);

		/* Determine the number of workable values. */
		for (i = 256; i-- && freqs_sorted[i].qsvalue > 0;) {
			++n;
		}
	}

	const unsigned int offset = (256 - n);

	unsigned int scanline_idx = 0;
	sc_huffman_node_t *scanline = calloc(n, sizeof(*scanline)), *current;
	for (i = offset; i < 256; ++i) {
		scanline_idx = (i - offset);
		fqp = &freqs_sorted[i];

		current = &scanline[scanline_idx];

		if (scanline_idx > 0) {
			current->previous = &scanline[scanline_idx - 1];
		}

		if (scanline_idx < (n - 1)) {
			current->next = &scanline[scanline_idx + 1];
		}

		current->node = sc_ll_node_alloc(
			fqp->qsvalue,
			fqp->tag,
			SC_LL_LEAF
		);
		context->tree_lookup[fqp->tag] = current->node;
	}


	unsigned int
		n_null = 0,
		end = (n - 1),
		left_idx = 0,
		right_idx = 0,
		next_idx = 0;

	sc_huffman_node_t
		*left = NULL,
		*right = NULL;

	sc_ll_node_t *nonleaf = NULL;

	do {
		for (i = 0; i < n; ++i) {
			current = &scanline[i];

			if (current->node == NULL) {
				continue;
			}

			if (left == NULL) {
				left = current;
				left_idx = i;
			} else if (right == NULL) {
				right = current;
				right_idx = i;
				break;
			}
		}

		if (right == NULL) {
			break;
		}

		left->node->flags |= SC_LL_LEFT;
		right->node->flags |= SC_LL_RIGHT;

		nonleaf = sc_ll_node_alloc_ex(
			(left->node->frequency + right->node->frequency),
			0,
			0,
			left->node,
			right->node
		);
		left->node->parent = right->node->parent = nonleaf;
		left->node = right->node = NULL;
		++n_null;

		// TODO: o=find_sorted_insert_point, memmove(left_idx - o-1, right_idx - o), sl[o]=nonleaf
		scanline[right_idx].node = nonleaf;

		left = right = NULL;
	} while (n_null < end);

	free(scanline);
	scanline = NULL;


	context->tree_root = nonleaf;


	return SC_E_SUCCESS;
}


sc_result_t
sc_huffman_tree_print(sc_huffman_t *context) {
	if (context == NULL) {
		return SC_E_NULL;
	}


	const sc_ll_node_t
		*const *const tree_lookup = (const sc_ll_node_t *const *const)context->tree_lookup,
		*node = NULL;


	// TODO: sort by freq

	register unsigned int i, j;
	for (i = 0; i < 256; ++i) {
		if (tree_lookup[i] != NULL) {
			printf("% 3u (%u)\t", i, context->frequencies[i]);

			size_t n = 0;
			node = tree_lookup[i];
			while ((node = node->parent) != NULL) {
				++n;
			}

			char binbuf[n + 1];

			node = tree_lookup[i];
			for (j = n; j--;) {
				if (node == NULL) {
					break;
				}

				/* Check if we reached the root of the tree. */
				if (node->parent == NULL) {
					break;
				}

				if ((node->flags & SC_LL_LEFT) == SC_LL_LEFT) {
					binbuf[j] = '1';
				} else if ((node->flags & SC_LL_RIGHT) == SC_LL_RIGHT) {
					binbuf[j] = '0';
				} else {
					fputs("?", stdout);
				}

				node = node->parent;
			}

			binbuf[n] = '\0';
			puts(binbuf);
		}
	}


	return SC_E_SUCCESS;
}
