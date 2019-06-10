#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef enum NodeType {
    EPSILON, // 何もしないで次に進むノード
    BRANCH,
    CHAR,
    ANYCHAR,
    MATCH, // ゴール
} NodeType;

typedef struct Node {
    NodeType type;
    struct Node* next;
    char c;               // CHARで使う
    struct Node* subNode; // BRANCHで使う

    struct Node* linkListNext; // 最後に削除するためにリストに繋げておく
} Node;

Node* findTail(Node* node) {
    while (node->next) {
        node = node->next;
    }
    return node;
}

Node* gNodes = NULL;

Node* createNode(NodeType type, Node* next, char c, Node* subNode) {
    Node* newNode = (Node*)malloc(sizeof(Node));

    newNode->type = type;
    newNode->next = next;
    newNode->c = c;
    newNode->subNode = subNode;

    newNode->linkListNext = gNodes;
    gNodes = newNode;

    return newNode;
}

void releaseNodes() {
    Node* node = gNodes;
    Node* next;
    do {
        next = node->linkListNext;
        free(node);
        node = next;
    } while(node);
}

typedef struct ParseResult {
    const char* p;
    Node* head;
    Node* tail;
} ParseResult;

ParseResult createParseResult(const char* p, Node* head, Node* tail) {
    ParseResult r = { p, head, tail };
    return r;
}

ParseResult parseRegexStr(const char* p, Node* head, Node **tail) {
    char c = *p;
    
    switch (c) {
        case '\0':
            return createParseResult(p, head, findTail(*tail));

        case '(':
        {
            Node* node = createNode(EPSILON, NULL, 0, NULL);
            ParseResult r = parseRegexStr(p+1, node, &node->next);
            
            Node** next = &findTail(*tail)->next;
            *next = r.head;
            return parseRegexStr(r.p, head, next);
        }
        
        case ')':
            return createParseResult(p+1, head, findTail(*tail));

        case '|':
            // A|B
            //           e     B
            //         + --> o ----+
            //         | e     A   v
            // -->     o --> o --> o
        {
            Node* node = createNode(EPSILON, NULL, 0, NULL);
            ParseResult r = parseRegexStr(p+1, node, &node->next);
            
            Node* branch = createNode(BRANCH, *tail, 0, node);
            
            Node* branchEnd = createNode(EPSILON, NULL, 0, NULL);
            findTail(*tail)->next = branchEnd;
            findTail(r.tail)->next = branchEnd;
            *tail = branch;
            
            return createParseResult(r.p, head, branchEnd);
        }
            
        case '+':
        {
            // A+     +-----+
            //        v  A  |   e
            // -->    o --> o   -->
            Node* node = createNode(BRANCH, NULL, 0, *tail);
            Node** next = &findTail(*tail)->next;
            *next = node;
            return parseRegexStr(p+1, head, next);
        }

        case '*':
        {
            // A*
            //               +------+
            //               v  c   |e
            // -->   (1) --> o --> (2)  -->
            //        |e            ^
            //        +-------------+
            Node* node2 = createNode(BRANCH, NULL, 0, *tail);
            Node** next = &findTail(*tail)->next;
            *next = node2;

            Node* node1 = createNode(BRANCH, node2, 0, *tail);
            *tail = node1;
            return parseRegexStr(p+1, head, next);

        }
        default:
        {
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9'))
            {
                Node* node = createNode(CHAR, NULL, c, NULL);
                Node** next = &findTail(*tail)->next;
                *next = node;
                return parseRegexStr(p+1, head, next);
                
            } else {
                // エラー
                return createParseResult(0, 0, 0);
            }
        }
            
    }
}

typedef struct State {
    Node *node;
    const char* p;
    struct State *prev; // stackに積むためのリンク
} State;

State* createState(Node *node, const char* p) {
    State* state = (State*)malloc(sizeof(State));
    state->node = node;
    state->p = p;
    
    return state;
}

State* gStateStack = NULL;

void pushState(State *state) {
    state->prev = gStateStack;
    gStateStack = state;
}

State* popState() {
    State* state = gStateStack;
    gStateStack = (state)?state->prev:NULL;
    return state;
}

bool match(const char *str, const char* regexStr) {
    
    Node* start = createNode(EPSILON, NULL, 0, NULL);
    ParseResult r = parseRegexStr(regexStr, start, &start);
    if (r.p == 0) {
        // エラー
        return false;
    }
    findTail(r.tail)->next = createNode(MATCH, NULL, 0, NULL);

    char *p = str;
    State *state = createState(start, p);

    while (true) {
        Node* node = state->node;
        switch (node->type) {
            case MATCH:
                return true;
            case EPSILON:
                state->node = node->next;
                continue;
            case BRANCH:
                state->node = node->next;
                pushState(createState(node->subNode, p));
                continue;
                
            case CHAR:
                if (*p == node->c) {
                    p++;
                    state->node = node->next;
                    continue;
                } else {
                    state = popState();
                    if (!state) {
                        return false;
                    }
                    continue;
                }
            case ANYCHAR:
                if (*p != '\0') {
                    p++;
                    state->node = node->next;
                    continue;
                } else {
                    state = popState();
                    if (!state) {
                        return false;
                    }
                    continue;
                }
            default:
                break;
                
        }
    }
    
    
    return true;
}



int main(int argc, const char * argv[]) {
    assert(match("a", "a"));
    assert(!match("b", "a"));

    return 0;
}

