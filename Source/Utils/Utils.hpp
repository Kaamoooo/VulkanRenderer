#include <typeinfo>
#include <string>

namespace Kaamoo {
    using id_t = unsigned int;
    
    class Utils {
    public:
        
        template<typename T>
        static std::string TypeName(){
            return typeid(T).name();
        }
    };
}