#include <iostream>
#include <vector>
#include <climits>

using namespace std;

struct Node {
    Node* left;
    Node* right;
    Node* up;
    Node* down;
    Node* column;
    int size;
    int row_id;
    Node() : left(nullptr), right(nullptr), up(nullptr), down(nullptr), column(nullptr), size(0), row_id(-1) {}
};

void cover(Node* c) {
    c->left->right = c->right;
    c->right->left = c->left;
    for (Node* i = c->down; i != c; i = i->down) {
        for (Node* j = i->right; j != i; j = j->right) {
            j->up->down = j->down;
            j->down->up = j->up;
            j->column->size--;
        }
    }
}

void uncover(Node* c) {
    for (Node* i = c->up; i != c; i = i->up) {
        for (Node* j = i->left; j != i; j = j->left) {
            j->column->size++;
            j->up->down = j;
            j->down->up = j;
        }
    }
    c->left->right = c;
    c->right->left = c;
}

Node* choose_column(Node* root) {
    int min = INT_MAX;
    Node* best = nullptr;
    for (Node* cur = root->right; cur != root; cur = cur->right) {
        if (cur->size < min && cur->size > 0) {
            min = cur->size;
            best = cur;
        }
    }
    return best;
}

bool solve(Node* root, vector<int>& solution) {
    Node* c = choose_column(root);
    if (c == nullptr) return true;  // All constraints satisfied
    cover(c);
    for (Node* r = c->down; r != c; r = r->down) {
        solution.push_back(r->row_id);
        for (Node* j = r->right; j != r; j = j->right) {
            cover(j->column);
        }
        if (solve(root, solution)) return true;
        solution.pop_back();
        for (Node* j = r->left; j != r; j = j->left) {
            uncover(j->column);
        }
    }
    uncover(c);
    return false;
}

int main() {
    vector<vector<int>> grid(9, vector<int>(9));
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            cin >> grid[i][j];
        }
    }

    // Root node for DLX
    Node* root = new Node();
    root->left = root;
    root->right = root;

    // Create column headers (324 constraints)
    vector<Node*> columns(324);
    for (int i = 0; i < 324; ++i) {
        Node* h = new Node();
        h->size = 0;
        h->row_id = -1;
        h->column = h;
        h->up = h;
        h->down = h;
        h->left = root->left;
        h->right = root;
        root->left->right = h;
        root->left = h;
        columns[i] = h;
    }

    // Create rows (729 possible placements) & link nodes
    vector<Node*> row_heads(729);
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            for (int k = 1; k <= 9; ++k) {
                int row_id = r * 81 + c * 9 + k - 1;
                int col_ids[4] = {
                    r * 9 + c,                  // Cell constraint
                    81 + r * 9 + k - 1,         // Row-number constraint
                    162 + c * 9 + k - 1,        // Column-number constraint
                    243 + ((r / 3) * 3 + (c / 3)) * 9 + k - 1  // Box-number constraint
                };
                Node* nodes[4];
                Node* first = nullptr;
                for (int idx = 0; idx < 4; ++idx) {
                    Node* n = new Node();
                    n->row_id = row_id;
                    n->column = columns[col_ids[idx]];
                    n->up = columns[col_ids[idx]]->up;
                    n->down = columns[col_ids[idx]];
                    columns[col_ids[idx]]->up->down = n;
                    columns[col_ids[idx]]->up = n;
                    columns[col_ids[idx]]->size++;
                    nodes[idx] = n;
                    if (idx == 0) first = n;
                }
                // Circular horizontal links for the row
                for (int idx = 0; idx < 4; ++idx) {
                    nodes[idx]->left = nodes[(idx + 3) % 4];
                    nodes[idx]->right = nodes[(idx + 1) % 4];
                }
                row_heads[row_id] = first;
            }
        }
    }

    // Solution vector (row IDs selected)
    vector<int> solution;

    // Pre-select rows for given clues
    bool valid = true;
    for (int r = 0; r < 9 && valid; ++r) {
        for (int c = 0; c < 9 && valid; ++c) {
            int k = grid[r][c];
            if (k != 0) {
                int row_id = r * 81 + c * 9 + k - 1;
                Node* r_node = row_heads[row_id];
                cover(r_node->column);
                for (Node* j = r_node->right; j != r_node; j = j->right) {
                    cover(j->column);
                }
                solution.push_back(row_id);
            }
        }
    }

    // Solve the remaining puzzle
    if (valid && solve(root, solution)) {
        // Decode solution to grid
        vector<vector<int>> solved(9, vector<int>(9, 0));
        for (int rid : solution) {
            int r = rid / 81;
            int c = (rid / 9) % 9;
            int k = (rid % 9) + 1;
            solved[r][c] = k;
        }
        // Output solved grid
        for (const auto& row : solved) {
            for (int num : row) {
                cout << num << " ";
            }
            cout << endl;
        }
    } else {
        cout << "No solution" << endl;
    }

    // Memory cleanup omitted for brevity (use smart pointers in production)

    return 0;
}