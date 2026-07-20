// Module.h
#ifndef MODULE_H
#define MODULE_H

class Module {
protected:
    int hand;  // Unique identifier or parameter for the module

public:
    // Constructor to initialize 'hand'
    Module(int hand) : hand(hand) {}

    // Pure virtual methods to be implemented by derived classes
    virtual void initialize() = 0;
    virtual void run() = 0;
    virtual void runFast() = 0;
    virtual void draw() = 0;

    // Virtual destructor for proper cleanup
    virtual ~Module() {}
};

#endif // MODULE_H
