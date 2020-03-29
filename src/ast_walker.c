#include <ravi_compiler.h>

#include <ravi_ast.h>

enum event_type {
	EV_START_CHUNK,
	EV_END_CHUNK,
	EV_START_TABLE_CONSTRUCTOR,
	EV_END_TABLE_CONSTRUCTOR,
	EV_INDEXED_ASSIGN_START,
	EV_INDEXED_ASSIGN_END,
	EV_INDEX_START,
	EV_INDEX_END,
	EV_VALUE_START,
	EV_VALUE_END,
	EV_Y_INDEX_START,
	EV_Y_INDEX_END,
	EV_FIELD_SELECTOR_START,
	EV_FIELD_SELECTOR_END,
	EV_UNARY_EXPRESSION_START,
	EV_UNARY_EXPRESSION_END,
	EV_BINARY_EXPRESSION_START,
	EV_BINARY_EXPRESSION_END,
	EV_SUFFIXED_EXPRESSION_START,
	EV_SUFFIXED_EXPRESSION_END,
	EV_PRIMARY_EXPRESSION_START,
	EV_PRIMARY_EXPRESSION_END,
	EV_SUFFIX_LIST_START,
	EV_SUFFIX_LIST_END,
	EV_FORNUM_STATEMENT_START,
	EV_FORNUM_STATEMENT_END,
	EV_FORNUM_SYMBOLS_START,
	EV_FORNUM_SYMBOLS_END,
	EV_FORNUM_EXPRESSIONS_START,
	EV_FORNUM_EXPRESSIONS_END,
	EV_FORNUM_BODY_START,
	EV_FORNUM_BODY_END,

};

struct event {
	enum event_type event_type;
	const struct var_type *var_type;
};

struct literal_event {
	ravitype_t type_code;
	SemInfo info;
};

struct visitor {
	void *userdata;
	void (*handle_event)(struct event event);
	void (*handle_literal)(struct literal_event event);
	void (*handle_unary_expr)(struct event event, UnOpr op);
	void (*handle_binary_expr)(struct event event, BinOpr op);
};

void raviX_walk_ast_node(struct ast_node *node, struct visitor *visitor);

static void walk_ast_node_list(struct ast_node_list *list, struct visitor *visitor)
{
	struct ast_node *node;
	bool is_first = true;
	FOR_EACH_PTR(list, node) { raviX_walk_ast_node(node, visitor); }
	END_FOR_EACH_PTR(node);
}

static void walk_statement_list(struct ast_node_list *statement_list, struct visitor *visitor)
{
	walk_ast_node_list(statement_list, visitor);
}

static void walk_symbol(struct lua_symbol *sym, struct visitor *visitor)
{
	switch (sym->symbol_type) {
	case SYM_GLOBAL: {
		// printf_buf(buf, "%p%t %c %s %s\n", level, sym->var.var_name, "global symbol",
		//	type_name(sym->value_type.type_code), get_as_str(sym->value_type.type_name));
		break;
	}
	case SYM_LOCAL: {
		// printf_buf(buf, "%p%t %c %s %s\n", level, sym->var.var_name, "local symbol",
		//	type_name(sym->value_type.type_code), get_as_str(sym->value_type.type_name));
		break;
	}
	case SYM_UPVALUE: {
		// printf_buf(buf, "%p%t %c %s %s\n", level, sym->upvalue.var->var.var_name, "upvalue",
		//	type_name(sym->upvalue.var->value_type.type_code),
		//	get_as_str(sym->upvalue.var->value_type.type_name));
		break;
	}
	default:
		assert(0);
	}
}

static void walk_symbol_name(struct lua_symbol *sym, struct visitor *visitor)
{
	switch (sym->symbol_type) {
	case SYM_LOCAL:
	case SYM_GLOBAL: {
		// printf_buf(buf, "%t", sym->var.var_name);
		break;
	}
	case SYM_UPVALUE: {
		// printf_buf(buf, "%t", sym->upvalue.var->var.var_name);
		break;
	}
	default:
		assert(0);
	}
}

static void walk_symbol_list(struct lua_symbol_list *list, struct visitor *visitor)
{
	struct lua_symbol *node;
	bool is_first = true;
	FOR_EACH_PTR(list, node) { walk_symbol(node, visitor); }
	END_FOR_EACH_PTR(node);
}

static void walk_symbol_names(struct lua_symbol_list *list, struct visitor *visitor)
{
	struct lua_symbol *node;
	FOR_EACH_PTR(list, node) { walk_symbol_name(node, visitor); }
	END_FOR_EACH_PTR(node);
}

void raviX_walk_ast_node(struct ast_node *node, struct visitor *visitor)
{
	switch (node->type) {
	case AST_FUNCTION_EXPR: {
		if (node->function_expr.args) {
			// printf_buf(buf, "%pfunction(\n", level);
			walk_symbol_list(node->function_expr.args, visitor);
			// printf_buf(buf, "%p)\n", level);
		} else {
			// printf_buf(buf, "%pfunction()\n", level);
		}
		if (node->function_expr.locals) {
			// printf_buf(buf, "%p%c ", level, "locals ");
			walk_symbol_names(node->function_expr.locals, visitor);
			// printf_buf(buf, "\n");
		}
		if (node->function_expr.upvalues) {
			// printf_buf(buf, "%p%c ", level, "upvalues ");
			walk_symbol_names(node->function_expr.upvalues, visitor);
			// printf_buf(buf, "\n");
		}
		walk_statement_list(node->function_expr.function_statement_list, visitor);
		// printf_buf(buf, "%pend\n", level);
		break;
	}
	case AST_NONE:
		break;
	case AST_RETURN_STMT: {
		// printf_buf(buf, "%preturn\n", level);
		walk_ast_node_list(node->return_stmt.expr_list, visitor);
		break;
	}
	case AST_LOCAL_STMT: {
		// printf_buf(buf, "%plocal\n", level);
		// printf_buf(buf, "%p%c\n", level, "[symbols]");
		walk_symbol_list(node->local_stmt.var_list, visitor);
		if (node->local_stmt.expr_list) {
			// printf_buf(buf, "%p%c\n", level, "[expressions]");
			walk_ast_node_list(node->local_stmt.expr_list, visitor);
		}
		break;
	}
	case AST_FUNCTION_STMT: {
		raviX_walk_ast_node(node->function_stmt.name, visitor);
		if (node->function_stmt.selectors) {
			// printf_buf(buf, "%p%c\n", level + 1, "[selectors]");
			walk_ast_node_list(node->function_stmt.selectors, visitor);
		}
		if (node->function_stmt.method_name) {
			// printf_buf(buf, "%p%c\n", level + 1, "[method name]");
			raviX_walk_ast_node(node->function_stmt.method_name, visitor);
		}
		// printf_buf(buf, "%p=\n", level + 1);
		raviX_walk_ast_node(node->function_stmt.function_expr, visitor);
		break;
	}
	case AST_LABEL_STMT: {
		// printf_buf(buf, "%p::%t::\n", level, node->label_stmt.symbol->label.label_name);
		break;
	}
	case AST_GOTO_STMT: {
		// printf_buf(buf, "%pgoto %t\n", level, node->goto_stmt.name);
		break;
	}
	case AST_DO_STMT: {
		// printf_buf(buf, "%pdo\n", level);
		walk_ast_node_list(node->do_stmt.do_statement_list, visitor);
		// printf_buf(buf, "%pend\n", level);
		break;
	}
	case AST_EXPR_STMT: {
		// printf_buf(buf, "%p%c\n", level, "[expression statement start]");
		if (node->expression_stmt.var_expr_list) {
			// printf_buf(buf, "%p%c\n", level + 1, "[var list start]");
			walk_ast_node_list(node->expression_stmt.var_expr_list, visitor);
			// printf_buf(buf, "%p= %c\n", level + 1, "[var list end]");
		}
		// printf_buf(buf, "%p%c\n", level + 1, "[expression list start]");
		walk_ast_node_list(node->expression_stmt.expr_list, visitor);
		// printf_buf(buf, "%p%c\n", level + 1, "[expression list end]");
		// printf_buf(buf, "%p%c\n", level, "[expression statement end]");
		break;
	}
	case AST_IF_STMT: {
		struct ast_node *test_then_block;
		bool is_first = true;
		FOR_EACH_PTR(node->if_stmt.if_condition_list, test_then_block)
		{
			// if (is_first) {
			//	is_first = false;
			//	printf_buf(buf, "%pif\n", level);
			//}
			// else
			//	printf_buf(buf, "%pelseif\n", level);
			raviX_walk_ast_node(test_then_block->test_then_block.condition, visitor);
			// printf_buf(buf, "%pthen\n", level);
			walk_ast_node_list(test_then_block->test_then_block.test_then_statement_list, visitor);
		}
		END_FOR_EACH_PTR(node);
		if (node->if_stmt.else_block) {
			// printf_buf(buf, "%pelse\n", level);
			walk_ast_node_list(node->if_stmt.else_statement_list, visitor);
		}
		// printf_buf(buf, "%pend\n", level);
		break;
	}
	case AST_WHILE_STMT: {
		// printf_buf(buf, "%pwhile\n", level);
		raviX_walk_ast_node(node->while_or_repeat_stmt.condition, visitor);
		// printf_buf(buf, "%pdo\n", level);
		walk_ast_node_list(node->while_or_repeat_stmt.loop_statement_list, visitor);
		// printf_buf(buf, "%pend\n", level);
		break;
	}
	case AST_REPEAT_STMT: {
		// printf_buf(buf, "%prepeat\n", level);
		walk_ast_node_list(node->while_or_repeat_stmt.loop_statement_list, visitor);
		// printf_buf(buf, "%puntil\n", level);
		raviX_walk_ast_node(node->while_or_repeat_stmt.condition, visitor);
		// printf_buf(buf, "%p%c\n", level, "[repeat end]");
		break;
	}
	case AST_FORIN_STMT: {
		// printf_buf(buf, "%pfor\n", level);
		walk_symbol_list(node->for_stmt.symbols, visitor);
		// printf_buf(buf, "%pin\n", level);
		walk_ast_node_list(node->for_stmt.expr_list, visitor);
		// printf_buf(buf, "%pdo\n", level);
		walk_statement_list(node->for_stmt.for_statement_list, visitor);
		// printf_buf(buf, "%pend\n", level);
		break;
	}
	case AST_FORNUM_STMT: {
		// printf_buf(buf, "%pfor\n", level);
		walk_symbol_list(node->for_stmt.symbols, visitor);
		// printf_buf(buf, "%p=\n", level);
		walk_ast_node_list(node->for_stmt.expr_list, visitor);
		// printf_buf(buf, "%pdo\n", level);
		walk_statement_list(node->for_stmt.for_statement_list, visitor);
		// printf_buf(buf, "%pend\n", level);
		break;
	}
	case AST_SUFFIXED_EXPR: {
		visitor->handle_event(
		    (struct event){.event_type = EV_SUFFIXED_EXPRESSION_START, .var_type = &node->suffixed_expr.type});
		visitor->handle_event((struct event){.event_type = EV_PRIMARY_EXPRESSION_START,
						     .var_type = &node->suffixed_expr.primary_expr->common_expr.type});
		raviX_walk_ast_node(node->suffixed_expr.primary_expr, visitor);
		visitor->handle_event((struct event){.event_type = EV_PRIMARY_EXPRESSION_END});
		if (node->suffixed_expr.suffix_list) {
			visitor->handle_event((struct event){.event_type = EV_SUFFIX_LIST_START});
			walk_ast_node_list(node->suffixed_expr.suffix_list, visitor);
			visitor->handle_event((struct event){.event_type = EV_SUFFIX_LIST_END});
		}
		visitor->handle_event((struct event){.event_type = EV_SUFFIXED_EXPRESSION_END});
		break;
	}
	case AST_FUNCTION_CALL_EXPR: {
		// printf_buf(buf, "%p%c %T\n", level, "[function call start]", &node->function_call_expr.type);
		if (node->function_call_expr.method_name) {
			// printf_buf(buf, "%p: %t (\n", level + 1, node->function_call_expr.method_name);
		} else {
			// printf_buf(buf, "%p(\n", level + 1);
		}
		walk_ast_node_list(node->function_call_expr.arg_list, visitor);
		// printf_buf(buf, "%p)\n", level + 1);
		// printf_buf(buf, "%p%c\n", level, "[function call end]");
		break;
	}
	case AST_SYMBOL_EXPR: {
		walk_symbol(node->symbol_expr.var, visitor);
		break;
	}
	case AST_BINARY_EXPR: {
		visitor->handle_binary_expr(
		    (struct event){.event_type = EV_BINARY_EXPRESSION_START, .var_type = &node->binary_expr.type},
		    node->binary_expr.binary_op);
		raviX_walk_ast_node(node->binary_expr.expr_left, visitor);
		raviX_walk_ast_node(node->binary_expr.expr_right, visitor);
		visitor->handle_binary_expr((struct event){.event_type = EV_BINARY_EXPRESSION_END},
					    node->binary_expr.binary_op);
		break;
	}
	case AST_UNARY_EXPR: {
		visitor->handle_unary_expr(
		    (struct event){.event_type = EV_UNARY_EXPRESSION_START, .var_type = &node->unary_expr.type},
		    node->unary_expr.unary_op);
		raviX_walk_ast_node(node->unary_expr.expr, visitor);
		visitor->handle_unary_expr((struct event){.event_type = EV_UNARY_EXPRESSION_END},
					   node->unary_expr.unary_op);
		break;
	}
	case AST_LITERAL_EXPR: {
		switch (node->literal_expr.type.type_code) {
		case RAVI_TNIL:
			visitor->handle_literal((struct literal_event){.type_code = node->literal_expr.type.type_code});
			break;
		case RAVI_TBOOLEAN:
			visitor->handle_literal((struct literal_event){.type_code = node->literal_expr.type.type_code,
								       .info.i = node->literal_expr.u.i});
			break;
		case RAVI_TNUMINT:
			visitor->handle_literal((struct literal_event){.type_code = node->literal_expr.type.type_code,
								       .info.i = node->literal_expr.u.i});
			break;
		case RAVI_TNUMFLT:
			visitor->handle_literal((struct literal_event){.type_code = node->literal_expr.type.type_code,
								       .info.r = node->literal_expr.u.n});
			break;
		case RAVI_TSTRING:
			visitor->handle_literal((struct literal_event){.type_code = node->literal_expr.type.type_code,
								       .info.ts = node->literal_expr.u.s});
			break;
		default:
			assert(0);
		}
		break;
	}
	case AST_FIELD_SELECTOR_EXPR: {
		visitor->handle_event(
		    (struct event){.event_type = EV_FIELD_SELECTOR_START, .var_type = &node->index_expr.type});
		raviX_walk_ast_node(node->index_expr.expr, visitor);
		visitor->handle_event((struct event){.event_type = EV_FIELD_SELECTOR_END});
		break;
	}
	case AST_Y_INDEX_EXPR: {
		visitor->handle_event(
		    (struct event){.event_type = EV_Y_INDEX_START, .var_type = &node->index_expr.type});
		raviX_walk_ast_node(node->index_expr.expr, visitor);
		visitor->handle_event((struct event){.event_type = EV_Y_INDEX_END});
		break;
	}
	case AST_INDEXED_ASSIGN_EXPR: {
		visitor->handle_event(
		    (struct event){.event_type = EV_INDEXED_ASSIGN_START, .var_type = &node->indexed_assign_expr.type});
		if (node->indexed_assign_expr.key_expr) {
			visitor->handle_event((struct event){.event_type = EV_INDEX_START});
			raviX_walk_ast_node(node->indexed_assign_expr.key_expr, visitor);
			visitor->handle_event((struct event){.event_type = EV_INDEX_END});
		}
		visitor->handle_event((struct event){.event_type = EV_VALUE_START});
		raviX_walk_ast_node(node->indexed_assign_expr.value_expr, visitor);
		visitor->handle_event((struct event){.event_type = EV_VALUE_END});
		visitor->handle_event(
		    (struct event){.event_type = EV_INDEXED_ASSIGN_END, .var_type = &node->indexed_assign_expr.type});
		break;
	}
	case AST_TABLE_EXPR: {
		visitor->handle_event(
		    (struct event){.event_type = EV_START_TABLE_CONSTRUCTOR, .var_type = &node->table_expr.type});
		walk_ast_node_list(node->table_expr.expr_list, visitor);
		visitor->handle_event(
		    (struct event){.event_type = EV_END_TABLE_CONSTRUCTOR, .var_type = &node->table_expr.type});
		break;
	}
	default:
		// printf_buf(buf, "%pUnsupported node type %d\n", level, node->type);
		assert(0);
	}
}

void raviX_walk_ast(struct compiler_state *container, struct visitor *visitor)
{
	visitor->handle_event((struct event){.event_type = EV_START_CHUNK});
	raviX_walk_ast_node(container->main_function, visitor);
	visitor->handle_event((struct event){.event_type = EV_END_CHUNK});
}
