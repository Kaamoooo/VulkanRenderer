#ifndef COMPONENT_INCLUDED
#define COMPONENT_INCLUDED

#include <string>
namespace Kaamoo{
    class Component{
    public:
        virtual std::string GetName(){return name;}

        virtual ~Component()=default;
    protected:
        std::string name="Component";
    };
}

#endif