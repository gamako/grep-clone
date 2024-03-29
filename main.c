#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

typedef enum NodeType {
    EPSILON, // 何もしないで次に進むノード
    BRANCH,
    CHAR,
    ANYCHAR,
    LINE_HEAD,
    LINE_TAIL,
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
    Node* next;
    do {
        next = gNodes->linkListNext;
        free(gNodes);
        gNodes = next;
    } while(gNodes);
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

// [...]のパース
ParseResult parseBracketRegexStr(const char* p) {
    
    Node *tail = createNode(EPSILON, NULL, 0, NULL);
    Node *head = NULL;
    Node **branchTail = NULL;

    while(true) {
        char c = *p;
        switch (c) {
            case ']':
                if (head == NULL) {
                    return createParseResult(0, NULL, NULL);
                }
                p++;

                return createParseResult(p, head, tail);
                
            default:
            {
                if ((c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9'))
                {
                    p++;
                    if (*p == '-') {
                        p++;
                        char c2 = *p;
                        if (c2 == '-' || c2 == ']' || c2 == c) {
                            return createParseResult(0, NULL, NULL);
                        }
                        for (char i = c; i <= c2; i++) {
                            Node *n = createNode(CHAR, tail, i, NULL);
                            if (head) {
                                Node *b = createNode(BRANCH, *branchTail, 0, n);
                                *branchTail = b;
                                branchTail = &(b->subNode);
                            } else {
                                head = n;
                                branchTail = &head;
                            }
                        }
                    } else {
                        Node *n = createNode(CHAR, tail, c, NULL);
                        if (head) {
                            Node *b = createNode(BRANCH, *branchTail, 0, n);
                            *branchTail = b;
                            branchTail = &(b->subNode);
                        } else {
                            head = n;
                            branchTail = &head;
                        }
                    }
                }
                break;
            }
        }
    }
}

ParseResult parseRegexStr(const char* p, Node* head, Node **tail) {
    char c = *p;
    
    switch (c) {
        case '\0':
            return createParseResult(p, head, findTail(*tail));

        case '(':
        {
            Node* node = createNode(EPSILON, NULL, 0, NULL);
            ParseResult r = parseRegexStr(p+1, node, &node);
            
            if (r.p == 0) {
                return r;
            }
            
            Node** next = &findTail(*tail)->next;
            *next = r.head;
            return parseRegexStr(r.p, head, next);
        }
        
        case ')':
            return createParseResult(p+1, head, findTail(*tail));

        case '[':
        {
            ParseResult r = parseBracketRegexStr(p+1);
            findTail(*tail)->next = r.head;
            
            return createParseResult(r.p, head, r.tail);
        }

        case '|':
            // A|B
            //           e     B
            //         + --> o ----+
            //         | e     A   v
            // -->     o --> o --> o
        {
            Node* node = createNode(EPSILON, NULL, 0, NULL);
            ParseResult r = parseRegexStr(p+1, node, &node);
            
            if (r.p == 0) {
                return r;
            }
            
            Node* branch = createNode(BRANCH, head, 0, r.head);
            
            Node* branchEnd = createNode(EPSILON, NULL, 0, NULL);
            findTail(*tail)->next = branchEnd;
            findTail(r.tail)->next = branchEnd;
            //*tail = branchEnd;
            
            return createParseResult(r.p, branch, branchEnd);
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
        case '.':
        {
            Node* node = createNode(ANYCHAR, NULL, c, NULL);
            Node** next = &findTail(*tail)->next;
            *next = node;
            return parseRegexStr(p+1, head, next);
        }
        case '^':
        {
            Node* node = createNode(LINE_HEAD, NULL, c, NULL);
            Node** next = &findTail(*tail)->next;
            *next = node;
            return parseRegexStr(p+1, head, next);
        }
        case '$':
        {
            Node* node = createNode(LINE_TAIL, NULL, c, NULL);
            Node** next = &findTail(*tail)->next;
            *next = node;
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
    bool isHead;
    struct State *prev; // stackに積むためのリンク
} State;

State* createState(Node *node, const char* p, bool isHead) {
    State* state = (State*)malloc(sizeof(State));
    state->node = node;
    state->p = p;
    state->isHead = isHead;
    
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

void clearStateStack() {
    State *state;
    while((state = popState())) {
        free(state);
    }
}

bool match(const char *str, const char* regexStr) {
    
    Node* node = createNode(EPSILON, NULL, 0, NULL);
    ParseResult r = parseRegexStr(regexStr, node, &node);
    if (r.p == 0) {
        // エラー
        return false;
    }
    findTail(r.tail)->next = createNode(MATCH, NULL, 0, NULL);
    Node* start = r.head;

    const char *startPos = str;
    bool isHead = true;

    while (true) {
        State *state = createState(start, startPos, isHead);
        
        while (true) {
            Node* node = state->node;
            switch (node->type) {
                case MATCH:
                    free(state);
                    clearStateStack();
                    releaseNodes();
                    return true;
                case EPSILON:
                    state->node = node->next;
                    continue;
                case BRANCH:
                    state->node = node->next;
                    pushState(createState(node->subNode, state->p, state->isHead));
                    continue;
                    
                case CHAR:
                    if (*(state->p) == node->c) {
                        state->p++;
                        state->node = node->next;
                        continue;
                    } else {
                        free(state);
                        state = popState();
                        if (!state) {
                            goto NOT_MATCH;
                        }
                        continue;
                    }
                case ANYCHAR:
                    if (*(state->p) != '\0') {
                        state->p++;
                        state->node = node->next;
                        continue;
                    } else {
                        free(state);
                        state = popState();
                        if (!state) {
                            goto NOT_MATCH;
                        }
                        continue;
                    }
                case LINE_HEAD:
                    if (state->isHead) {
                        state->node = node->next;
                        continue;
                    } else {
                        free(state);
                        state = popState();
                        if (!state) {
                            goto NOT_MATCH;
                        }
                        continue;
                    }
                case LINE_TAIL:
                    if (*(state->p) == '\0') {
                        state->node = node->next;
                        continue;
                    } else {
                        free(state);
                        state = popState();
                        if (!state) {
                            goto NOT_MATCH;
                        }
                        continue;
                    }
            }
        }
        
    NOT_MATCH:
        
        if (!*startPos) {
            break;
        }
        
        startPos++;
        isHead = false;
    }

    clearStateStack();
    releaseNodes();
    return false;
}

int test() {
    
    assert(!match("", "a"));
    assert(match("a", "a"));
    assert(!match("b", "a"));

    assert(!match("", "ab"));
    assert(!match("a", "ab"));
    assert(!match("b", "ab"));
    assert(match("ab", "ab"));
    assert(!match("ac", "ab"));
    assert(match("abc", "ab"));
    assert(match("cab", "ab"));
    assert(match("cabd", "ab"));
    assert(match("aabd", "ab"));
    assert(!match("caed", "ab"));
    assert(!match("aaed", "ab"));

    // +
    assert(!match("", "a+"));
    assert(match("a", "a+"));
    assert(!match("b", "a+"));

    assert(!match("a", "a+b"));
    assert(!match("b", "a+b"));
    assert(match("ab", "a+b"));
    assert(!match("ac", "a+b"));
    assert(match("abc", "a+b"));
    assert(match("cab", "a+b"));
    assert(match("cabd", "a+b"));
    assert(match("aabd", "a+b"));
    assert(!match("caed", "a+b"));
    assert(!match("aaed", "a+b"));

    assert(!match("a", "ab+"));
    assert(!match("b", "ab+"));
    assert(match("ab", "ab+"));
    assert(!match("ac", "ab+"));
    assert(match("abc", "ab+"));
    assert(match("cab", "ab+"));
    assert(match("cabd", "ab+"));
    assert(match("aabd", "ab+"));
    assert(!match("caed", "ab+"));
    assert(!match("aaed", "ab+"));

    assert(!match("a", "a+b+"));
    assert(!match("b", "a+b+"));
    assert(match("ab", "a+b+"));
    assert(!match("ac", "a+b+"));
    assert(match("abc", "a+b+"));
    assert(match("cab", "a+b+"));
    assert(match("cabd", "a+b+"));
    assert(match("aabd", "a+b+"));
    assert(!match("caed", "a+b+"));
    assert(!match("aaed", "a+b+"));

    // *
    assert(match("", "a*"));
    assert(match("a", "a*"));
    assert(match("b", "a*"));
    
    assert(!match("", "a*b"));
    assert(!match("a", "a*b"));
    assert(match("b", "a*b"));
    assert(match("ab", "a*b"));
    assert(match("aab", "a*b"));
    assert(match("aaab", "a*b"));
    assert(match("baab", "a*b"));
    assert(match("caab", "a*b"));
    assert(match("cb", "a*b"));
    assert(match("cbd", "a*b"));

    assert(!match("", "ab*"));
    assert(match("a", "ab*"));
    assert(match("ab", "ab*"));
    assert(match("aab", "ab*"));
    assert(match("aaab", "ab*"));
    assert(match("baab", "ab*"));
    assert(match("baabb", "ab*"));
    assert(match("baabc", "ab*"));

    assert(!match("", "ab*c"));
    assert(!match("a", "ab*c"));
    assert(!match("b", "ab*c"));
    assert(!match("c", "ab*c"));
    assert(!match("ab", "ab*c"));
    assert(match("ac", "ab*c"));
    assert(match("abc", "ab*c"));
    assert(match("abbc", "ab*c"));
    assert(match("dac", "ab*c"));
    assert(match("dabc", "ab*c"));
    assert(match("dabbc", "ab*c"));

    // .
    assert(!match("", "."));
    assert(match("a", "."));
    assert(match("b", "."));
    assert(!match("", ".+"));
    assert(match("a", ".+"));
    assert(match("aa", ".+"));
    assert(match("ab", ".+"));
    assert(match("", ".*"));
    assert(match("a", ".*"));

    // |
    assert(match("a", "a|b"));
    assert(match("b", "a|b"));
    assert(!match("c", "a|b"));
    assert(!match("a", "ab|cd"));
    assert(match("ab", "ab|cd"));
    assert(match("cd", "ab|cd"));

    // ()
    assert(match("abc", "a(bc)+"));
    assert(!match("abc", "a(bc)+d"));
    assert(match("abcd", "a(bc)+d"));
    assert(match("abcbcd", "a(bc)+d"));

    assert(match("abcf", "a(bc|de)f"));
    assert(match("adef", "a(bc|de)f"));
    assert(!match("abcdf", "a(bc|de)f"));
    assert(!match("adegf", "a(bc|de)f"));
    
    assert(match("abch", "a(bc|de|fg)h"));
    assert(match("adeh", "a(bc|de|fg)h"));
    assert(match("afgh", "a(bc|de|fg)h"));

    assert(match("abh", "a(bc*|de*|fg*)h"));
    assert(match("abch", "a(bc*|de*|fg*)h"));
    assert(match("abcch", "a(bc*|de*|fg*)h"));
    assert(match("adh", "a(bc*|de*|fg*)h"));
    assert(match("adeh", "a(bc*|de*|fg*)h"));
    assert(match("adeeh", "a(bc*|de*|fg*)h"));
    assert(match("afh", "a(bc*|de*|fg*)h"));
    assert(match("afgh", "a(bc*|de*|fg*)h"));
    assert(match("afggh", "a(bc*|de*|fg*)h"));

    // ^, $
    assert(match("a", "^a$"));
    
    // [...]
    assert(match("a", "[a]"));
    assert(!match("b", "[a]"));
    assert(match("a", "[ab]"));
    assert(match("b", "[ab]"));
    assert(match("a", "[a-c]"));
    assert(match("b", "[a-c]"));
    assert(match("c", "[a-c]"));
    assert(match("abe", "a[b-d]e"));
    assert(match("ace", "a[b-d]e"));
    assert(match("ade", "a[b-d]e"));
    assert(match("ade", "a[b-d]+"));
    assert(match("abe", "a[b-d]+[e-g]+"));
    assert(match("acf", "a[b-d]+[e-g]+"));
    assert(match("adg", "a[b-d]+[e-g]+"));

    return 0;
}

int main(int argc, const char * argv[]) {
    
    
    if (argc < 2) {
        printf("run test\n");
        test();
        return 0;
    }
    const char* regex = argv[1];
    
    char buffer[200];
    while ((fgets(buffer, 200, stdin))) {
        if (match(buffer, regex)) {
            printf("%s", buffer);
        }
    }
}
