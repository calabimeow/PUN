#include "pun.h"

static int offset;
static int stack_size;

#ifdef _WIN32
    static char *accreg = "rcx";
    static char *argreg64[] = {"rcx", "rdx", "r8", "r9"};
#else
    static char *accreg = "rdi";
    static char *argreg64[] = {"rdi", "rsi", "rdx", "rcx"};
#endif

static void push()
{
    fprintf(output_file, "    push rax\n");
}

static void pop(char *arg)
{
    fprintf(output_file, "    pop %s\n", arg);
}

static int count()
{
    static int i = 1;
    return i++;
}

static void gen_addr(node *n)
{
    fprintf(output_file, "    lea rax, [rbp%d]\n", n->var->offset);
}

static void gen_expr(node *n)
{
    switch (n->kind)
    {
        case NODE_NUM:
            fprintf(output_file, "    mov rax, %d\n", n->val);
            return;
        case NODE_VAR:
            gen_addr(n);
            fprintf(output_file, "    mov rax, [rax]\n");
            return;
       case NODE_ASSIGN:
            gen_addr(n->lhs);
            push();
            gen_expr(n->rhs);
            pop(accreg);
            fprintf(output_file, "    mov [%s], rax\n", accreg);
            return;

        case NODE_FUNCALL:
        {
            size_t nargs = n->args ? n->args->size : 0;
            size_t i = 0;

            for (i = 0; i < nargs; i++)
            {
                gen_expr(&n->args->nodes[i]);
                push();
            }

            for (i = nargs; i > 0; i--)
                pop(argreg64[i - 1]);

            fprintf(output_file, "    call \"%s\"\n", n->funcname);
            return;
        }

        default: break;
    }

    gen_expr(n->rhs);
    push();
    gen_expr(n->lhs);
    pop(accreg);

    switch (n->kind)
    {
        case NODE_ADD:
            fprintf(output_file, "    add rax, %s\n", accreg);
            return;
        case NODE_SUB:
            fprintf(output_file, "    sub rax, %s\n", accreg);
            return;
        case NODE_MUL:
            fprintf(output_file, "    imul rax, %s\n", accreg);
            return;
        case NODE_DIV:
            fprintf(output_file, "    cqo\n");
            fprintf(output_file, "    idiv %s\n", accreg);
            return;
        case NODE_EQ:
        case NODE_NE:
        case NODE_LT:
        case NODE_LE:
            fprintf(output_file, "    cmp rax, %s\n", accreg);

            if (n->kind == NODE_EQ)
                fprintf(output_file, "    sete al\n");
            else if (n->kind == NODE_NE)
                fprintf(output_file, "    setne al\n");
            else if (n->kind == NODE_LT)
                fprintf(output_file, "    setl al\n");
            else if (n->kind == NODE_LE)
                fprintf(output_file, "    setle al\n");

            fprintf(output_file, "    movzb rax, al\n");

        default: break;
    }
}

static void gen_stmt(node *n)
{
    switch (n->kind)
    {
        case NODE_BLOCK:
            if (n->body)
            {   
                size_t i;
                for (i = 0; i < n->body->size; i++) 
                    gen_stmt(&n->body->nodes[i]);
            }
            return;

        case NODE_ASSIGN:
            gen_expr(n);
            return;
        case NODE_RETURN:
            gen_expr(n->lhs);
            fprintf(output_file, "    jmp .L.return\n");
            return;

        case NODE_IF:
        {
            int c = count();
            gen_expr(n->cond);
            fprintf(output_file, "    cmp rax, 0\n");
            fprintf(output_file, "    je .L.else.%d\n", c);
            gen_stmt(n->then);
            fprintf(output_file, "    jmp .L.end.%d\n", c);
            fprintf(output_file, ".L.else.%d:\n", c);

            if (n->els)
                gen_stmt(n->els);

            fprintf(output_file, ".L.end.%d:\n", c);
            return;
        }

        case NODE_FOR:
        {
            int c = count();
            gen_stmt(n->init);
            fprintf(output_file, ".L.begin.%d:\n", c);
            
            gen_expr(n->cond);
            fprintf(output_file, "    cmp rax, 0\n");
            fprintf(output_file, "    je .L.end.%d\n", c);

            gen_stmt(n->then);
            gen_expr(n->inc);

            fprintf(output_file, "    jmp .L.begin.%d\n", c);
            fprintf(output_file, "    .L.end.%d:\n", c);
            return;
        }

        case NODE_PRINT:
            gen_expr(n->lhs);
            fprintf(output_file, "    mov %s, rax\n", accreg);
            fprintf(output_file, "    call putchar\n");
            return;

        case NODE_LABEL:
            fprintf(output_file, "\"%s\":\n", n->label);
            return;

        case NODE_JUMP:
            fprintf(output_file, "    jmp \"%s\"\n", n->label);
            return;

        case NODE_FUNCALL:
        {
            size_t nargs = n->args ? n->args->size : 0;
            size_t i = 0;

            for (i = 0; i < nargs; i++)
            {
                gen_expr(&n->args->nodes[i]);
                push();
            }

            for (i = nargs; i > 0; i--)
                pop(argreg64[i - 1]);

            fprintf(output_file, "    call \"%s\"\n", n->funcname);
            return;
        }

        default: break;
    }
}

static int align_to(int n, int align)
{
    return (n + align - 1) / align * align;
}

static void assign_offsets(node *n)
{
    if (!n)
        return;

    if (n->kind == NODE_VAR)
    {
        if (n->var->offset == 0)
        {
            offset += 8;
            n->var->offset = -offset;
        }

        return;
    }

    assign_offsets(n->lhs);
    assign_offsets(n->rhs);
    assign_offsets(n->cond);
    assign_offsets(n->then);
    assign_offsets(n->els);
    assign_offsets(n->init);
    assign_offsets(n->inc);

    if (n->body)
    {
        size_t i;
        for (i = 0; i < n->body->size; i++)
            assign_offsets(&n->body->nodes[i]);
    }

    if (n->args)
    {
        size_t i = 0;
        for (i = 0; i < n->args->size; i++)
            assign_offsets(&n->args->nodes[i]);
    }

    if (n->params)
    {
        size_t i = 0;
        for (i = 0; i < n->params->size; i++)
            assign_offsets(&n->params->nodes[i]);
    }
}

static void assign_lvar_offsets(node_arr *na)
{
    size_t i = 0;

    for (i = 0; i < na->size; i++)
    {
        node *n = &na->nodes[i];
        if (n->kind == NODE_FUNC)
        {
            offset = 0;
            if (n->params)
                for (i = 0; i < n->params->size; i++)
                    assign_offsets(&n->params->nodes[i]);

            assign_offsets(n->func);
            n->val = align_to(offset, 16);
        }
    }

    for (i = 0; i < na->size; i++)
        if (na->nodes[i].kind != NODE_FUNC)
            assign_offsets(&na->nodes[i]);

    stack_size = align_to(offset, 16);
}

void codegen(node_arr *na)
{
    size_t i;
    assign_lvar_offsets(na);

    fprintf(output_file, ".intel_syntax noprefix\n");

    for (i = 0; i < na->size; i++)
    {
        node *n = &na->nodes[i];

        if (n->kind == NODE_FUNC)
        {
            fprintf(output_file, ".global \"%s\"\n", n->funcname);
            fprintf(output_file, "\"%s\":\n", n->funcname);

            fprintf(output_file, "    push rbp\n");
            fprintf(output_file, "    mov rbp, rsp\n");
            fprintf(output_file, "    sub rsp, %d\n", n->val);

            if (n->params)
            {
                size_t j = 0;
                for (j = 0; j < n->params->size; j++)
                {
                    node *p = &n->params->nodes[j];
                    fprintf(output_file, "    mov [rbp%d], %s\n", p->var->offset, argreg64[j]);
                }
            }

            gen_stmt(n->func);

            fprintf(output_file, ".L.return.%s:\n", n->funcname);
            fprintf(output_file, "    mov rsp, rbp\n");
            fprintf(output_file, "    pop rbp\n");
            fprintf(output_file, "    ret\n");
        }
    }

    fprintf(output_file, ".global main\n");
    fprintf(output_file, "main:\n");

    fprintf(output_file, "    push rbp\n");
    fprintf(output_file, "    mov rbp, rsp\n");
    fprintf(output_file, "    sub rsp, %d\n", stack_size);

    for (i = 0; i < na->size; i++)
        gen_stmt(&na->nodes[i]);

    fprintf(output_file, "    xor rax, rax\n");
    fprintf(output_file, ".L.return:\n");
    fprintf(output_file, "    mov rsp, rbp\n");
    fprintf(output_file, "    pop rbp\n");
    fprintf(output_file, "    ret\n");
}
