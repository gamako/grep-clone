#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

typedef enum NodeType {
    EMPTY,
    CHAR,
    BRANCH,
    MATCH,
} NodeType;

typedef struct Node {
    NodeType type;
    char c;
    struct Node* next;
    struct Node* branch;
} Node;

Node* createNode(NodeType type) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->type = type;
    node->next = NULL;
    return node;
}

const char* parseRegex(const char* regex, Node** head, Node** tail) {

    const char *p = regex;
    Node* nodeHead = createNode(EMPTY);
    Node* nodeTail = nodeHead;

    while (true) {
        switch (*p) {
            case '\0':
                goto LOOP_END;
            
            case '|':
            {
                p++;
                Node *branchHead, *branchTail;
                p = parseRegex(p, &branchHead, &branchTail);
                
                Node *branchNode = createNode(BRANCH);
                
                branchNode->next = nodeHead;
                branchNode->branch = branchHead;
                
                Node *end = createNode(EMPTY);
                
                nodeTail->next = end;
                branchTail->next = end;
                
                nodeHead = branchNode;
                nodeTail = end;
            }
                
                break;
                
            default:
            {
                // その他文字
                Node* node = createNode(CHAR);
                node->c = *p;
                p++;
                
                switch (*p) {
                    case '*':
                    {
                        Node* branch1 = createNode(BRANCH);
                        Node* branch2 = createNode(BRANCH);
                        Node* end = createNode(EMPTY);
                        
                        branch1->next = node;
                        branch1->branch = end;
                        
                        node->next = branch2;
                        
                        branch2->next = end;
                        branch2->branch = node;
                        
                        nodeTail->next = branch1;
                        nodeTail = end;
                        
                        p++;
                    }
                        break;
                        
                    default:
                        nodeTail->next = node;
                        nodeTail = node;
                        break;
                }
            }
                break;
        }
    }
LOOP_END:

    *head = nodeHead;
    *tail = nodeTail;
    
    return p;
}

typedef struct State {
    Node* node;
    const char* p;
    struct State *link;
} State;

State* createState(Node* node, const char* p) {
    State* state = (State*)malloc(sizeof(State));
    state->node = node;
    state->p = p;
    state->link = NULL;
    
    return state;
}

State *stateStackTop = NULL;

void pushState(State *state) {
    state->link = stateStackTop;
    stateStackTop = state;
}

State* popState() {
    State *state = stateStackTop;
    if (state != NULL) {
        stateStackTop = state->link;
        state->link = NULL;
    }
    return state;
}

// 正規表現の判定
bool isMatchRegex(const char* str, const char* regexStr) {

    Node *nodeHead, *nodeTail;
    parseRegex(regexStr, &nodeHead, &nodeTail);
    nodeTail->next = createNode(MATCH);

    const char* startStr = str;
    while (true) {
        const char *p = startStr;
        Node* node = nodeHead;
        
        while (true) {
            switch (node->type) {
                case EMPTY:
                    node = node->next;
                    break;
                case BRANCH:
                {
                    State* state = createState(node->branch, p);
                    pushState(state);
                    
                    node = node->next;
                }
                    
                    break;
                case CHAR:
                    if (*p == node->c) {
                        node = node->next;
                        p++;
                    } else {
                        State *state = popState();
                        if (state == NULL) {
                            goto LOOP_NEXT;
                        } else {
                            node = state->node;
                            p = state->p;
                        }
                    }
                    break;
                case MATCH:
                    return true;
                    
                default:
                    assert(false);
            }
        }
    LOOP_NEXT:
        startStr++;
        
        if (*startStr == '\0') {
            break;
        }
    }
    
    return false;
}

void test() {
    assert(isMatchRegex("a", "a"));
    assert(!isMatchRegex("b", "a"));

    assert(isMatchRegex("ab", "ab"));
    assert(!isMatchRegex("ac", "ab"));

    assert(isMatchRegex("ab", "b"));

    assert(isMatchRegex("a", "a|b"));
    assert(isMatchRegex("b", "a|b"));
    assert(isMatchRegex("abc", "abc|cde"));
    assert(isMatchRegex("cde", "abc|cde"));
    assert(!isMatchRegex("cd", "abc|cde"));

    assert(isMatchRegex("ac", "ab*c"));
    assert(isMatchRegex("abc", "ab*c"));
    assert(isMatchRegex("abbc", "ab*c"));
    
}

int main(int argc, const char * argv[]) {
    test();
    return 0;
}
