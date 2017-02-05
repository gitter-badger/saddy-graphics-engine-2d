#include "dukpp-03/context.h"

#include <sadpoint.h>

#include <renderer.h>
#include <util/fs.h>


DECLARE_COMMON_TYPE(sad::dukpp03::CompiledFunction)

// ============================================ PUBLIC METHODS ============================================

sad::dukpp03::Context::Context(bool vanilla) : m_renderer(NULL), m_vanilla(vanilla)
{
    if (!m_vanilla)
    {
        this->initialize();
    }    
}

sad::dukpp03::Context::~Context()
{
    
}

bool sad::dukpp03::Context::evalFromFile(
    const std::string& path, 
    bool clean_heap,
    std::string* error
)
{
    // Test if file exists
    sad::String mpath = path.c_str();
    if (sad::util::fileExists(mpath.c_str()) == false)
    {
        sad::Renderer* r = m_renderer;
        if (!r)
        {
            r = sad::Renderer::ref();
        }
        if (sad::util::isAbsolutePath(mpath) == false)
        {
             mpath = util::concatPaths(r->executablePath(), mpath);
             if (sad::util::fileExists(mpath.c_str()) == false)
             {
                if (error) 
                {
                    *error = "File \"";
                    *error += path;
                    *error += "\" does not exist";
                }
                return false;
             }
        }
        else
        {
            if (error) 
            {
                *error = "File \"";
                *error += path;
                *error += "\" does not exist";
            }
            return false;
        }
    }
    m_running = true;
    startEvaluating();
    bool result = false;
    if (duk_peval_file(m_context, mpath.c_str()) != 0) 
    {
        if (error)
        {
            *error = duk_safe_to_string(m_context, -1);
        }
        duk_pop(m_context);
    } 
    else 
    {
        if (error)
        {
            *error = "";
        }
        result = true;
        if (clean_heap)
        {
            duk_pop(m_context);
        }
    }
    m_running = false;
    return result;
    
}

void sad::dukpp03::Context::setRenderer(sad::Renderer* r)
{
    m_renderer = r;
}

sad::Renderer* sad::dukpp03::Context::renderer() const
{
    return m_renderer;
}

void sad::dukpp03::Context::reset()
{
    this->sad::dukpp03::BasicContext::reset();
    if (!m_vanilla)
    {
        this->initialize();
    }
}


bool sad::dukpp03::Context::addClassBinding(const std::string& name, ::dukpp03::ClassBinding<Self>* c)
{
    if (this->sad::dukpp03::BasicContext::addClassBinding(name, c))
    {
        if (name.find("*") == std::string::npos)
        {
            std::string ptrname = name;
            ptrname += " *";
            this->sad::dukpp03::BasicContext::addClassBinding(ptrname, new ::dukpp03::ClassBinding<Self>(*c));
        }
        return true;
    }
    return false;
}

// ============================================ PROTECTED METHODS ============================================

extern const std::string __context_eval_info;


static duk_ret_t isNativeObject(duk_context* ctx)
{
    int count = duk_get_top(ctx);
    if (count != 1)
    {
        duk_push_error_object(ctx, DUK_ERR_TYPE_ERROR, "isNativeObject: got %d arguments, instead of 1", count);
        duk_throw(ctx);
        return 0;
    }
    duk_bool_t result = 0;
    if (duk_is_object(ctx, 0))
    {
        result = duk_has_prop_string(ctx, 0, DUKPP03_VARIANT_PROPERTY_SIGNATURE);
    }
    duk_push_boolean(ctx, result);
    return 1;
}

sad::String dumpNativeObject(const sad::db::Variant& v)
{
    sad::Maybe<sad::db::Object*> maybeobject = v.get<sad::db::Object*>();
    if (maybeobject.exists())
    {
        std::stringstream ss;
        ss << maybeobject.value()->serializableName();
        ss << "(" << maybeobject.value() << ")";
        std::string name = ss.str(); 
        return name;
    }
    else
    {
        std::stringstream ss;
        ss << v.typeName();
        ss << "(" << v.data() << ")";
        std::string name = ss.str(); 
        return name;
    }
}

// ReSharper disable once CppMemberFunctionMayBeStatic
// ReSharper disable once CppMemberFunctionMayBeConst
void sad::dukpp03::Context::initialize()
{
    // Register SadInternalIsNativeObject function
    duk_push_global_object(m_context);
    duk_push_c_function(m_context, isNativeObject, 1);
    duk_int_t result = duk_peval_string(m_context, "new Function()");
    assert(result == 0);
    duk_dup(m_context, -1);
    duk_put_prop_string(m_context, -3, "prototype");
    duk_set_prototype(m_context, - 2);   
    duk_put_prop_string(m_context, -2 /*idx:global*/, "SadInternalIsNativeObject");
    duk_pop(m_context);  

    // Register SadInternalDumpNativeObject function
    this->registerCallable("SadInternalDumpNativeObject", sad::dukpp03::make_function::from(dumpNativeObject));

    this->exposePoint2D();
    this->exposePoint3D();
    this->exposePoint2I();
    this->exposePoint3I();

    std::string error;
    bool ok =  this->eval(__context_eval_info, true, &error);
    assert( ok );
}


void sad::dukpp03::Context::exposePoint2D()
{
    sad::dukpp03::ClassBinding* c = new sad::dukpp03::ClassBinding();
    c->addConstructor<sad::Point2D>("SadPoint2D");
    c->addConstructor<sad::Point2D, double, double>("SadPoint2D");
    c->addAccessor("x", sad::dukpp03::bind_method::from(&sad::Point2D::x), sad::dukpp03::bind_method::from(&sad::Point2D::setX));
    c->addAccessor("y", sad::dukpp03::bind_method::from(&sad::Point2D::y), sad::dukpp03::bind_method::from(&sad::Point2D::setY));
    c->addMethod("distance", sad::dukpp03::bind_method::from(&sad::Point2D::distance));
    c->addCloneValueObjectMethodFor<sad::Point2D>();

    this->addClassBinding("sad::Point2D", c);
}

void sad::dukpp03::Context::exposePoint3D()
{
    sad::dukpp03::ClassBinding* c = new sad::dukpp03::ClassBinding();
    c->addConstructor<sad::Point3D>("SadPoint3D");
    c->addConstructor<sad::Point3D, double, double, double>("SadPoint3D");
    c->addAccessor("x", sad::dukpp03::rebind_method::to<sad::Point3D>::from(&sad::Point3D::x), sad::dukpp03::rebind_method::to<sad::Point3D>::from(&sad::Point3D::setX));
    c->addAccessor("y", sad::dukpp03::rebind_method::to<sad::Point3D>::from(&sad::Point3D::y), sad::dukpp03::rebind_method::to<sad::Point3D>::from(&sad::Point3D::setY));
    c->addAccessor("z", sad::dukpp03::bind_method::from(&sad::Point3D::z), sad::dukpp03::bind_method::from(&sad::Point3D::setZ));
    c->addMethod("distance", sad::dukpp03::bind_method::from(&sad::Point3D::distance));
    c->addCloneValueObjectMethodFor<sad::Point3D>();

    this->addClassBinding("sad::Point3D", c);    
}

void sad::dukpp03::Context::exposePoint2I()
{
    sad::dukpp03::ClassBinding* c = new sad::dukpp03::ClassBinding();
    c->addConstructor<sad::Point2I>("SadPoint2I");
    c->addConstructor<sad::Point2I, int, int>("SadPoint2I");
    c->addAccessor("x", sad::dukpp03::bind_method::from(&sad::Point2I::x), sad::dukpp03::bind_method::from(&sad::Point2I::setX));
    c->addAccessor("y", sad::dukpp03::bind_method::from(&sad::Point2I::y), sad::dukpp03::bind_method::from(&sad::Point2I::setY));
    c->addMethod("distance", sad::dukpp03::bind_method::from(&sad::Point2I::distance));
    c->addCloneValueObjectMethodFor<sad::Point2I>();

    this->addClassBinding("sad::Point2I", c);       
}

void sad::dukpp03::Context::exposePoint3I()
{
    sad::dukpp03::ClassBinding* c = new sad::dukpp03::ClassBinding();
    c->addConstructor<sad::Point3I>("SadPoint3I");
    c->addConstructor<sad::Point3I, int, int, int>("SadPoint3I");
    c->addAccessor("x", sad::dukpp03::rebind_method::to<sad::Point3I>::from(&sad::Point2I::x), sad::dukpp03::rebind_method::to<sad::Point3I>::from(&sad::Point2I::setX));
    c->addAccessor("y", sad::dukpp03::rebind_method::to<sad::Point3I>::from(&sad::Point2I::y), sad::dukpp03::rebind_method::to<sad::Point3I>::from(&sad::Point2I::setY));
    c->addAccessor("z", sad::dukpp03::bind_method::from(&sad::Point3I::z), sad::dukpp03::bind_method::from(&sad::Point3I::setZ));
    c->addMethod("distance", sad::dukpp03::bind_method::from(&sad::Point3I::distance));
    c->addCloneValueObjectMethodFor<sad::Point3I>();

    this->addClassBinding("sad::Point3I", c);  
}