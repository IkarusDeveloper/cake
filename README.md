# CAKE: **C**reate **A**nd **K**eep **E**lements


How to use **owner_ptr** and **weak_ptr**
```c++
// how to create a new element
auto ownp = cake::make_owner<std::string>();
// owner pointers can be copied exactly how std::shared_ptr allow
auto ownp_copy = ownp; // <--- it increase the ref count

// ....
// how to share the object using weak pointers
auto weak = cake::make_weak(ownp); // <--- it increase the weakref count
auto weak2 = weak; // <--- it increase the weakref count as well

// how to get back the ownership using a weak_ptr
auto ownp_back = cake::get_ownership(weak); // <--- it increases the ref count
if(ownp_back == nullptr)
    std::cout << "ownership failed, object was cancelled!" << std::endl;
```


How to use **enable_proxy_from_this** and **proxy_ptr**
```c++
struct MyProxableObject : public cake::enable_proxy_from_this<MyProxableObject> {
  std::string name;
  std::string surname;
};

MyProxableObject obj;
obj.name = "Foo";
obj.surname = "Bar";

// how to generate a proxy_ptr
auto proxy = obj.proxy(); // <--- this proxy_ptr will remain alive till the object instance does exist
```
