#include "hexastore.h"

void* _hx_add_triple_threaded (void* arg);

hx_hexastore* hx_new_hexastore ( void ) {
	hx_hexastore* hx	= (hx_hexastore*) calloc( 1, sizeof( hx_hexastore ) );
	hx->spo			= hx_new_index( HX_INDEX_ORDER_SPO );
	hx->sop			= hx_new_index( HX_INDEX_ORDER_SOP );
	hx->pso			= hx_new_index( HX_INDEX_ORDER_PSO );
	hx->pos			= hx_new_index( HX_INDEX_ORDER_POS );
	hx->osp			= hx_new_index( HX_INDEX_ORDER_OSP );
	hx->ops			= hx_new_index( HX_INDEX_ORDER_OPS );
	return hx;
}

int hx_free_hexastore ( hx_hexastore* hx ) {
	hx_free_index( hx->spo );
	hx_free_index( hx->sop );
	hx_free_index( hx->pso );
	hx_free_index( hx->pos );
	hx_free_index( hx->osp );
	hx_free_index( hx->ops );
	free( hx );
	return 0;
}

int hx_add_triple( hx_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o ) {
	hx_terminal* t;
	
	{
		int added	= hx_index_add_triple_terminal( hx->spo, s, p, o, &t );
		hx_index_add_triple_with_terminal( hx->pso, t, s, p, o, added );
	}

	{
		int added	= hx_index_add_triple_terminal( hx->sop, s, p, o, &t );
		hx_index_add_triple_with_terminal( hx->osp, t, s, p, o, added );
	}
	
	{
		int added	= hx_index_add_triple_terminal( hx->pos, s, p, o, &t );
		hx_index_add_triple_with_terminal( hx->ops, t, s, p, o, added );
	}
	
	return 0;
}

int hx_add_triples( hx_hexastore* hx, hx_triple* triples, int count ) {
	if (count < THREADED_BATCH_SIZE) {
		for (int i = 0; i < count; i++) {
			hx_add_triple( hx, triples[i].subject, triples[i].predicate, triples[i].object );
		}
	} else {
		pthread_t threads[3];
		hx_thread_info tinfo[3];
		for (int i = 0; i < 3; i++) {
			tinfo[i].count		= count;
			tinfo[i].triples	= triples;
		}

		{
			tinfo[0].index		= hx->spo;
			tinfo[0].secondary	= hx->pso;
			
			tinfo[1].index		= hx->sop;
			tinfo[1].secondary	= hx->osp;
			
			tinfo[2].index		= hx->pos;
			tinfo[2].secondary	= hx->ops;
			
			for (int i = 0; i < 3; i++) {
				pthread_create(&(threads[i]), NULL, _hx_add_triple_threaded, &( tinfo[i] ));
			}
			for (int i = 0; i < 3; i++) {
				pthread_join(threads[i], NULL);
			}
		}
	}
	return 0;
}

void* _hx_add_triple_threaded (void* arg) {
	hx_thread_info* tinfo	= (hx_thread_info*) arg;
	for (int i = 0; i < tinfo->count; i++) {
		hx_node_id s	= tinfo->triples[i].subject;
		hx_node_id p	= tinfo->triples[i].predicate;
		hx_node_id o	= tinfo->triples[i].object;
		hx_terminal* t;
		int added	= hx_index_add_triple_terminal( tinfo->index, s, p, o, &t );
		hx_index_add_triple_with_terminal( tinfo->secondary, t, s, p, o, added );
	}
	return NULL;
}

int hx_remove_triple( hx_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o ) {
	hx_index_remove_triple( hx->spo, s, p, o );
	hx_index_remove_triple( hx->sop, s, p, o );
	hx_index_remove_triple( hx->pso, s, p, o );
	hx_index_remove_triple( hx->pos, s, p, o );
	hx_index_remove_triple( hx->osp, s, p, o );
	hx_index_remove_triple( hx->ops, s, p, o );
	return 0;
}

int hx_get_ordered_index( hx_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o, int order_position, hx_index** index, hx_node_id* nodes ) {
	int i		= 0;
	int vars	= 0;
//	fprintf( stderr, "triple: { %d, %d, %d }\n", (int) s, (int) p, (int) o );
	int used[3]	= { 0, 0, 0 };
	hx_node_id triple[3]	= { s, p, o };
	int index_order[3]		= { 0xdeadbeef, 0xdeadbeef, 0xdeadbeef };
	char* pnames[3]			= { "SUBJECT", "PREDICATE", "OBJECT" };
	
	if (s > (hx_node_id) 0) {
//		fprintf( stderr, "- bound subject\n" );
		index_order[ i++ ]	= HX_SUBJECT;
		used[ HX_SUBJECT ]++;
	} else if (s < (hx_node_id) 0) {
		vars++;
	}
	if (p > (hx_node_id) 0) {
//		fprintf( stderr, "- bound predicate\n" );
		index_order[ i++ ]	= HX_PREDICATE;
		used[ HX_PREDICATE ]++;
	} else if (p < (hx_node_id) 0) {
		vars++;
	}
	if (o > (hx_node_id) 0) {
//		fprintf( stderr, "- bound object\n" );
		index_order[ i++ ]	= HX_OBJECT;
		used[ HX_OBJECT ]++;
	} else if (o < (hx_node_id) 0) {
		vars++;
	}
	
//	fprintf( stderr, "index order: { %d, %d, %d }\n", (int) index_order[0], (int) index_order[1], (int) index_order[2] );
	if (i < 3 && !(used[order_position]) && triple[order_position] != (hx_node_id) 0) {
//		fprintf( stderr, "requested ordering position: %s\n", pnames[order_position] );
		index_order[ i++ ]	= order_position;
		used[order_position]++;
	}
//	fprintf( stderr, "index order: { %d, %d, %d }\n", (int) index_order[0], (int) index_order[1], (int) index_order[2] );
	
	// check for any duplicated variables. if they haven't been added to the index order, add them now:
	for (int j = 0; j < 3; j++) {
		if (!(used[j])) {
			int current_chosen	= i;
//			fprintf( stderr, "checking if %s (%d) matches already chosen nodes:\n", pnames[j], (int) triple[j] );
			for (int k = 0; k < current_chosen; k++) {
//				fprintf( stderr, "- %s (%d)?\n", pnames[k], triple[ index_order[k] ] );
				if (triple[index_order[k]] == triple[j] && triple[j] != (hx_node_id) 0) {
//					fprintf( stderr, "*** MATCHED\n" );
					if (i < 3) {
						index_order[ i++ ]	= j;
						used[ j ]++;
					}
				}
			}
		}
	}
	
	// add any remaining triple positions to the index order:
	if (i == 0) {
		for (int j = 0; j < 3; j++) {
			if (j != order_position) {
				index_order[ i++ ]	= j;
			}
		}
	} else if (i == 1) {
		for (int j = 0; j < 3; j++) {
			if (j != order_position && !(used[j])) {
				index_order[ i++ ]	= j;
			}
		}
	} else if (i == 2) {
		for (int j = 0; j < 3; j++) {
			if (!(used[j])) {
				index_order[ i++ ]	= j;
			}
		}
	}
	
	switch (index_order[0]) {
		case 0:
			switch (index_order[1]) {
				case 1:
//					fprintf( stderr, "using spo index\n" );
					*index	= hx->spo;
					break;
				case 2:
//					fprintf( stderr, "using sop index\n" );
					*index	= hx->sop;
					break;
			}
			break;
		case 1:
			switch (index_order[1]) {
				case 0:
//					fprintf( stderr, "using pso index\n" );
					*index	= hx->pso;
					break;
				case 2:
//					fprintf( stderr, "using pos index\n" );
					*index	= hx->pos;
					break;
			}
			break;
		case 2:
			switch (index_order[1]) {
				case 0:
//					fprintf( stderr, "using osp index\n" );
					*index	= hx->osp;
					break;
				case 1:
//					fprintf( stderr, "using ops index\n" );
					*index	= hx->ops;
					break;
			}
			break;
	}
	
	hx_node_id triple_ordered[3]	= { s, p, o };
	nodes[0]	= triple_ordered[ (*index)->order[0] ];
	nodes[1]	= triple_ordered[ (*index)->order[1] ];
	nodes[2]	= triple_ordered[ (*index)->order[2] ];
	return 0;
}

hx_index_iter* hx_get_statements( hx_hexastore* hx, hx_node_id s, hx_node_id p, hx_node_id o, int order_position ) {
	hx_node_id index_ordered[3];
	hx_index* index;
	hx_get_ordered_index( hx, s, p, o, order_position, &index, index_ordered );
	hx_index_iter* iter	= hx_index_new_iter1( index, index_ordered[0], index_ordered[1], index_ordered[2] );
	return iter;
}

uint64_t hx_triples_count ( hx_hexastore* hx ) {
	hx_index* i	= hx->spo;
	return hx_index_triples_count( i );
}

int hx_write( hx_hexastore* h, FILE* f ) {
	fputc( 'X', f );
	if ((
		(hx_index_write( h->spo, f )) ||
		(hx_index_write( h->sop, f )) ||
		(hx_index_write( h->pso, f )) ||
		(hx_index_write( h->pos, f )) ||
		(hx_index_write( h->osp, f )) ||
		(hx_index_write( h->ops, f ))
		) != 0) {
		fprintf( stderr, "*** Error while writing hexastore indices to disk.\n" );
		return 1;
	} else {
		return 0;
	}
}

hx_hexastore* hx_read( FILE* f, int buffer ) {
	size_t read;
	int c	= fgetc( f );
	if (c != 'X') {
		fprintf( stderr, "*** Bad header cookie trying to read hexastore from file.\n" );
		return NULL;
	}
	hx_hexastore* hx	= (hx_hexastore*) calloc( 1, sizeof( hx_hexastore ) );
	hx->spo	= hx_index_read( f, buffer );
	hx->sop	= hx_index_read( f, buffer );
	hx->pso	= hx_index_read( f, buffer );
	hx->pos	= hx_index_read( f, buffer );
	hx->osp	= hx_index_read( f, buffer );
	hx->ops	= hx_index_read( f, buffer );
	if ((hx->spo == NULL) || (hx->spo == NULL) || (hx->spo == NULL) || (hx->spo == NULL) || (hx->spo == NULL) || (hx->spo == NULL)) {
		fprintf( stderr, "*** NULL index returned while trying to read hexastore from disk.\n" );
		free( hx );
		return NULL;
	} else {
		return hx;
	}
}
