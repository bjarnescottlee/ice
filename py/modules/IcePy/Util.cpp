// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Util.h>
#include <Identity.h>
#include <Ice/LocalException.h>

using namespace std;

IcePy::PyObjectHandle::PyObjectHandle(PyObject* p) :
    _p(p)
{
}

IcePy::PyObjectHandle::PyObjectHandle(const PyObjectHandle& p) :
    _p(p._p)
{
    Py_XINCREF(_p);
}

IcePy::PyObjectHandle::~PyObjectHandle()
{
    if(_p)
    {
        Py_DECREF(_p);
    }
}

void
IcePy::PyObjectHandle::operator=(PyObject* p)
{
    if(_p)
    {
        Py_DECREF(_p);
    }
    _p = p;
}

void
IcePy::PyObjectHandle::operator=(const PyObjectHandle& p)
{
    Py_XDECREF(_p);
    _p = p._p;
    Py_XINCREF(_p);
}

PyObject*
IcePy::PyObjectHandle::get()
{
    return _p;
}

PyObject*
IcePy::PyObjectHandle::release()
{
    PyObject* result = _p;
    _p = NULL;
    return result;
}

IcePy::AllowThreads::AllowThreads()
{
    _state = PyEval_SaveThread();
}

IcePy::AllowThreads::~AllowThreads()
{
    PyEval_RestoreThread(_state);
}

IcePy::AdoptThread::AdoptThread()
{
    _state = PyGILState_Ensure();
}

IcePy::AdoptThread::~AdoptThread()
{
    PyGILState_Release(_state);
}

bool
IcePy::listToStringSeq(PyObject* list, Ice::StringSeq& seq)
{
    assert(PyList_Check(list));

    int sz = PyList_Size(list);
    for(int i = 0; i < sz; ++i)
    {
        PyObject* item = PyList_GetItem(list, i);
        if(item == NULL)
        {
            return false;
        }
        char* str = PyString_AsString(item);
        if(str == NULL)
        {
            return false;
        }
        seq.push_back(str);
    }

    return true;
}

bool
IcePy::stringSeqToList(const Ice::StringSeq& seq, PyObject* list)
{
    assert(PyList_Check(list));

    for(Ice::StringSeq::const_iterator p = seq.begin(); p != seq.end(); ++p)
    {
        PyObject* str = Py_BuildValue("s", p->c_str());
        if(str == NULL)
        {
            Py_DECREF(list);
            return false;
        }
        int status = PyList_Append(list, str);
        Py_DECREF(str); // Give ownership to the list.
        if(status < 0)
        {
            Py_DECREF(list);
            return false;
        }
    }

    return true;
}

bool
IcePy::dictionaryToContext(PyObject* dict, Ice::Context& context)
{
    assert(PyDict_Check(dict));

    int pos = 0;
    PyObject* key;
    PyObject* value;
    while(PyDict_Next(dict, &pos, &key, &value))
    {
        char* keystr = PyString_AsString(key);
        if(keystr == NULL)
        {
            PyErr_Format(PyExc_ValueError, "context key must be a string");
            return false;
        }
        char* valuestr = PyString_AsString(value);
        if(valuestr == NULL)
        {
            PyErr_Format(PyExc_ValueError, "context value must be a string");
            return false;
        }
        context.insert(Ice::Context::value_type(keystr, valuestr));
    }

    return true;
}

bool
IcePy::contextToDictionary(const Ice::Context& ctx, PyObject* dict)
{
    assert(PyDict_Check(dict));

    for(Ice::Context::const_iterator p = ctx.begin(); p != ctx.end(); ++p)
    {
        PyObjectHandle key = PyString_FromString(const_cast<char*>(p->first.c_str()));
        PyObjectHandle value = PyString_FromString(const_cast<char*>(p->second.c_str()));
        if(key.get() == NULL || value.get() == NULL)
        {
            return false;
        }
        if(PyDict_SetItem(dict, key.get(), value.get()) < 0)
        {
            return false;
        }
    }

    return true;
}

bool
IcePy::splitString(const string& str, Ice::StringSeq& args)
{
    string delim = " \t\n\r";
    string::size_type beg;
    string::size_type end = 0;
    while(true)
    {
        beg = str.find_first_not_of(delim, end);
        if(beg == string::npos)
        {
            break;
        }

        //
        // Check for quoted argument.
        //
        char ch = str[beg];
        if(ch == '"' || ch == '\'')
        {
            beg++;
            end = str.find(ch, beg);
            if(end == string::npos)
            {
                PyErr_Format(PyExc_RuntimeError, "unterminated quote in `%s'", str.c_str());
                return false;
            }
            args.push_back(str.substr(beg, end - beg));
            end++; // Skip end quote.
        }
        else
        {
            end = str.find_first_of(delim + "'\"", beg);
            if(end == string::npos)
            {
                end = str.length();
            }
            args.push_back(str.substr(beg, end - beg));
        }
    }

    return true;
}

string
IcePy::scopedToName(const string& scoped)
{
    string result = fixIdent(scoped);
    if(result.find("::") == 0)
    {
        result.erase(0, 2);
    }

    string::size_type pos;
    while((pos = result.find("::")) != string::npos)
    {
        result.replace(pos, 2, ".");
    }

    return result;
}

static string
lookupKwd(const string& name)
{
    //
    // Keyword list. *Must* be kept in alphabetical order.
    //
    static const string keywordList[] = 
    {       
        "and", "assert", "break", "class", "continue", "def", "del", "elif", "else", "except", "exec",
        "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "not", "or", "pass",
        "print", "raise", "return", "try", "while", "yield"
    };
    bool found =  binary_search(&keywordList[0],
                                &keywordList[sizeof(keywordList) / sizeof(*keywordList)],
                                name);
    return found ? "_" + name : name;
}

//
// Split a scoped name into its components and return the components as a list of (unscoped) identifiers.
//
static Ice::StringSeq
splitScopedName(const string& scoped)
{
    assert(scoped[0] == ':');
    Ice::StringSeq ids;
    string::size_type next = 0;
    string::size_type pos;
    while((pos = scoped.find("::", next)) != string::npos)
    {
        pos += 2;
        if(pos != scoped.size())
        {
            string::size_type endpos = scoped.find("::", pos);
            if(endpos != string::npos)
            {
                ids.push_back(scoped.substr(pos, endpos - pos));
            }
        }
        next = pos;
    }
    if(next != scoped.size())
    {
        ids.push_back(scoped.substr(next));
    }
    else
    {
        ids.push_back("");
    }

    return ids;
}

string
IcePy::fixIdent(const string& ident)
{
    if(ident[0] != ':')
    {
        return lookupKwd(ident);
    }
    Ice::StringSeq ids = splitScopedName(ident);
    transform(ids.begin(), ids.end(), ids.begin(), ptr_fun(lookupKwd));
    stringstream result;
    for(Ice::StringSeq::const_iterator i = ids.begin(); i != ids.end(); ++i)
    {
        result << "::" + *i;
    }
    return result.str();
}

PyObject*
IcePy::lookupType(const string& typeName)
{
    string::size_type dot = typeName.rfind('.');
    string moduleName;
    string name;
    if(dot == string::npos)
    {
        moduleName = "__main__";
        name = typeName;
    }
    else
    {
        moduleName = typeName.substr(0, dot);
        name = typeName.substr(dot + 1);
    }

    PyObjectHandle module = PyImport_ImportModule(const_cast<char*>(moduleName.c_str()));
    if(module.get() == NULL)
    {
        return NULL;
    }

    PyObject* dict = PyModule_GetDict(module.get());
    assert(dict);

    return PyDict_GetItemString(dict, const_cast<char*>(name.c_str()));
}

PyObject*
IcePy::getPythonException(bool clear)
{
    PyObject* t;
    PyObject* val;
    PyObject* tb;

    PyErr_Fetch(&t, &val, &tb); // PyErr_Fetch clears the exception.
    PyErr_NormalizeException(&t, &val, &tb);

    if(clear)
    {
        Py_XDECREF(t);
        Py_XDECREF(tb);
    }
    else
    {
        Py_XINCREF(val);
        PyErr_Restore(t, val, tb);
    }

    return val; // New reference.
}

PyObject*
IcePy::createExceptionInstance(PyObject* type)
{
    assert(PyClass_Check(type));
    IcePy::PyObjectHandle args = PyTuple_New(0);
    if(args.get() == NULL)
    {
        return NULL;
    }
    return PyEval_CallObject(type, args.get());
}

void
IcePy::setPythonException(const Ice::Exception& ex)
{
    PyObjectHandle p;
    PyObject* type;

    ostringstream ostr;
    ostr << ex;
    string str = ostr.str();

    try
    {
        ex.ice_throw();
    }
    catch(const Ice::AlreadyRegisteredException& e)
    {
        type = lookupType("Ice.AlreadyRegisteredException");
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle s;
            s = PyString_FromString(const_cast<char*>(e.kindOfObject.c_str()));
            PyObject_SetAttrString(p.get(), "kindOfObject", s.get());
            s = PyString_FromString(const_cast<char*>(e.id.c_str()));
            PyObject_SetAttrString(p.get(), "id", s.get());
        }
    }
    catch(const Ice::NotRegisteredException& e)
    {
        type = lookupType("Ice.NotRegisteredException");
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle s;
            s = PyString_FromString(const_cast<char*>(e.kindOfObject.c_str()));
            PyObject_SetAttrString(p.get(), "kindOfObject", s.get());
            s = PyString_FromString(const_cast<char*>(e.id.c_str()));
            PyObject_SetAttrString(p.get(), "id", s.get());
        }
    }
    catch(const Ice::RequestFailedException& e)
    {
        type = lookupType(scopedToName(e.ice_name()));
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle m;
            m = createIdentity(e.id);
            PyObject_SetAttrString(p.get(), "id", m.get());
            m = PyString_FromString(const_cast<char*>(e.facet.c_str()));
            PyObject_SetAttrString(p.get(), "facet", m.get());
            m = PyString_FromString(const_cast<char*>(e.operation.c_str()));
            PyObject_SetAttrString(p.get(), "operation", m.get());
        }
    }
    catch(const Ice::UnknownException& e)
    {
        type = lookupType(scopedToName(e.ice_name()));
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle s = PyString_FromString(const_cast<char*>(e.unknown.c_str()));
            PyObject_SetAttrString(p.get(), "unknown", s.get());
        }
    }
    catch(const Ice::NoObjectFactoryException& e)
    {
        type = lookupType(scopedToName(e.ice_name()));
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle m;
            m = PyString_FromString(const_cast<char*>(e.type.c_str()));
            PyObject_SetAttrString(p.get(), "type", m.get());
        }
    }
    catch(const Ice::LocalException& e)
    {
        type = lookupType(scopedToName(e.ice_name()));
        if(type != NULL)
        {
            p = createExceptionInstance(type);
        }
        else
        {
            type = lookupType("Ice.UnknownLocalException");
            assert(type != NULL);
            p = createExceptionInstance(type);
            if(p.get() != NULL)
            {
                PyObjectHandle s = PyString_FromString(const_cast<char*>(str.c_str()));
                PyObject_SetAttrString(p.get(), "unknown", s.get());
            }
        }
    }
    catch(const Ice::UserException& e)
    {
        type = lookupType("Ice.UnknownUserException");
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle s = PyString_FromString(const_cast<char*>(str.c_str()));
            PyObject_SetAttrString(p.get(), "unknown", s.get());
        }
    }
    catch(const Ice::Exception& e)
    {
        type = lookupType("Ice.UnknownException");
        assert(type != NULL);
        p = createExceptionInstance(type);
        if(p.get() != NULL)
        {
            PyObjectHandle s = PyString_FromString(const_cast<char*>(str.c_str()));
            PyObject_SetAttrString(p.get(), "unknown", s.get());
        }
    }

    if(p.get() != NULL)
    {
        Py_INCREF(type);
        PyErr_Restore(type, p.release(), NULL);
    }
}

static void
throwLocalException(PyObject* ex)
{
    string typeName = ex->ob_type->tp_name;

    try
    {
        if(typeName == "Ice.ObjectNotExistException")
        {
            throw Ice::ObjectNotExistException(__FILE__, __LINE__);
        }
        else if(typeName == "Ice.OperationNotExistException")
        {
            throw Ice::OperationNotExistException(__FILE__, __LINE__);
        }
        else if(typeName == "Ice.FacetNotExistException")
        {
            throw Ice::FacetNotExistException(__FILE__, __LINE__);
        }
        else if(typeName == "Ice.RequestFailedException")
        {
            throw Ice::RequestFailedException(__FILE__, __LINE__);
        }
    }
    catch(Ice::RequestFailedException& e)
    {
        IcePy::PyObjectHandle member;
        member = PyObject_GetAttrString(ex, "id");
        if(member.get() != NULL && IcePy::checkIdentity(member.get()))
        {
            IcePy::getIdentity(member.get(), e.id);
        }
        member = PyObject_GetAttrString(ex, "facet");
        if(member.get() != NULL && PyString_Check(member.get()))
        {
            e.facet = PyString_AS_STRING(member.get());
        }
        member = PyObject_GetAttrString(ex, "operation");
        if(member.get() != NULL && PyString_Check(member.get()))
        {
            e.operation = PyString_AS_STRING(member.get());
        }
        throw e;
    }

    try
    {
        if(typeName == "Ice.UnknownLocalException")
        {
            throw Ice::UnknownLocalException(__FILE__, __LINE__);
        }
        else if(typeName == "Ice.UnknownUserException")
        {
            throw Ice::UnknownUserException(__FILE__, __LINE__);
        }
        else if(typeName == "Ice.UnknownException")
        {
            throw Ice::UnknownException(__FILE__, __LINE__);
        }
    }
    catch(Ice::UnknownException& e)
    {
        IcePy::PyObjectHandle member;
        member = PyObject_GetAttrString(ex, "unknown");
        if(member.get() != NULL && PyString_Check(member.get()))
        {
            e.unknown = PyString_AS_STRING(member.get());
        }
        throw e;
    }

    Ice::UnknownLocalException e(__FILE__, __LINE__);
    e.unknown = typeName;
    throw e;
}

void
IcePy::throwPythonException(PyObject* ex)
{
    PyObjectHandle h;
    if(ex == NULL)
    {
        h = getPythonException();
        ex = h.get();
    }

    PyObject* userExceptionType = lookupType("Ice.UserException");
    PyObject* localExceptionType = lookupType("Ice.LocalException");

    if(PyObject_IsInstance(ex, userExceptionType))
    {
        PyObjectHandle id = PyObject_CallMethod(ex, "ice_id", NULL);
        PyErr_Clear();
        Ice::UnknownUserException e(__FILE__, __LINE__);
        if(id.get() == NULL)
        {
            e.unknown = ex->ob_type->tp_name;
        }
        else
        {
            e.unknown = PyString_AS_STRING(id.get());
        }
        throw e;
    }
    else if(PyObject_IsInstance(ex, localExceptionType))
    {
        throwLocalException(ex);
    }
    else
    {
        PyObjectHandle str = PyObject_Str(ex);
        assert(str.get() != NULL);
        assert(PyString_Check(str.get()));

        Ice::UnknownException e(__FILE__, __LINE__);
        e.unknown = PyString_AS_STRING(str.get());
        throw e;
    }
}

void
IcePy::handleSystemExit()
{
    PyObjectHandle ex = getPythonException();
    assert(ex.get() != NULL);

    //
    // This code is similar to handle_system_exit in pythonrun.c.
    //
    PyObjectHandle code;
    if(PyInstance_Check(ex.get()))
    {
        code = PyObject_GetAttrString(ex.get(), "code");
    }
    else
    {
        code = ex;
    }

    int status;
    if(PyInt_Check(code.get()))
    {
        status = static_cast<int>(PyInt_AsLong(code.get()));
    }
    else
    {
        PyObject_Print(code.get(), stderr, Py_PRINT_RAW);
        PySys_WriteStderr("\n");
        status = 1;
    }

    code = 0;
    ex = 0;
    Py_Exit(status);
}
