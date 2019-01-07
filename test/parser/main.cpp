#include <iostream>
using namespace std;
struct node {
    char name[10];
    node() {
        strcpy(name, "default");
        cout << name << "construct\n";
    }
    ~node() {
        cout << name << "destroy\n";
    }
};
node getNode() {
    node a;
    strcpy(a.name, "a");
    return a;
}
int main() {
    node b;
    strcpy(b.name, "b");
    b = getNode();
    strcpy(b.name, "c");
    return 0;
}