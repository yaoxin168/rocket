#include <iostream>

class A{
public:
    static void newA(){
        
    }
    void setA(int a){
        m=a;
    }

private:
    int m=0;
};

int main(){
    
    A::setA(3);


    return 0;
}