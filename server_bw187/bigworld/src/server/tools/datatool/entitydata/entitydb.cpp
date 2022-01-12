#include "entitydb.hpp"
#ifdef _WIN32
#include <direct.h> 
#endif
#include "xmldatabase.h"

/** ���ݶ���ȫ�ֶ���ָ�� */
CXmlDatabase *g_pDatabases = NULL;

/** 
 * ��ʼ�����ýű�ģ��,script.cpp����ʵ�� 
 * �ڽ�����������Math��Resgr��ģ��ͺ���
 */
extern void runInitTimeJobs();


/**
 * ��ȡʵ�嶨�������ݿ��е�����
 * @param entName ʵ������
 * @param dbid ���ݿ�����ݱ��,����Ÿ�-1ʱ��ʹ��Ĭ��ֵ�����µ�����
 * @return  �ɹ�����ȡ�����������ݻ��߷���NULL
 */
DataSectionPtr openEntity2Section(const std::string &entName, int dbid)
{
    return g_pDatabases->openEntity(entName, dbid);
}

/**
 *  ����ʵ�����ݵ����ݿ�
 *  @prarm section ʵ�����ݶ���
 *  @prarm dbid ��������ݿ���
 *  @return ������������-1���������ݱ��, ���ظ�����ʾ�д���.
 */
int saveDataSection(DataSectionPtr section, int dbid)
{
    return g_pDatabases->saveEntity(section, dbid);
}

/**
 *  ReaMgr.saveEntity �ű��ӿڳ�ʼ��ʵ�嶨����������ݿ� 
 *  @prarm pObject ��ResMgr.openEntity �򿪵�ʵ������
 *  @return �������ݱ��
 */
static PyObject  *saveEntity( PyObjectPtr pObject, int dbid = -1 )
{
	PyDataSection *pOwner = reinterpret_cast<PyDataSection*>( pObject.getObject());
    return  Script::getData(saveDataSection(pOwner->pSection(), dbid));
}
PY_AUTO_MODULE_FUNCTION( RETDATA, saveEntity,
        ARG( PyObjectPtr, ARG( int, END ) ), ResMgr )

/** 
 *  ResMgr.openEntity ��ȡ���ݽű��ӿ� 
 *  @prarm args (��������,���ݿ�id)
 *  @return ����PyDataSection����
 */
static PyObject  *openEntity( const std::string &entName, int dbid = -1 )
{
    return new PyDataSection(openEntity2Section(entName, dbid) ,
            &PyDataSection::s_type_);
}
PY_AUTO_MODULE_FUNCTION( RETDATA, openEntity,
        ARG( std::string, ARG( int, END ) ), ResMgr )


/**
 *  ReaMgr.deleteEntity ɾ������
 */
static PyObject *deleteEntity(const std::string &name, int dbid)
{
    g_pDatabases->deleteEntity(name, dbid);
    Py_Return;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, deleteEntity,
        ARG( std::string, ARG( int, END ) ), ResMgr )


/**
 *  ReaMgr.getData ��ȡ��
 *  @return �ɹ������ֵ���������
 */
static PyObject *getData( const std::string &name, int seq )
{
    //�жϽṹ�Ƿ�Ķ�
    Py_Return;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, getData,
        ARG( std::string, ARG( int, END ) ), ResMgr )

/**
 *  ReaMgr.updateData ����xml�����ļ���ʹ���ݸ��µ����µĶ���ṹ��
 */
static PyObject *updateData( const std::string &name )
{
    g_pDatabases->updateDefinData(name);
    Py_Return;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, updateData,
        ARG( std::string, END), ResMgr )

/**
 *  ReaMgr.saveData ����xml�����ļ�
 */
static PyObject *saveData( const std::string &name )
{
    g_pDatabases->save(name);
    Py_Return;
}
PY_AUTO_MODULE_FUNCTION( RETDATA, saveData,
        ARG( std::string, END), ResMgr )

/**
 *  ReaMgr.init �ű��ӿڳ�ʼ��ʵ�嶨����������ݿ� 
 *  @prarm args (���paths.xml�ļ���·��)
 *  @return ���ؼ��سɹ���ʧ��
 *  - 0 �ɹ�
 *  - -1 ������������
 *  - -2 ��ʼ��BW��Դʧ��
 */
static PyObject *loadEntityDB(PyObject *self, PyObject *args)
{
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path))
    {
        return  Script::getData(-1);
    }

    //��ΪBWResource::init��ʹ�� ·�� + paths.xml ����Ҫ���·���Ƿ���"/"
    std::string basePath = path;
    std::replace(basePath.begin(), basePath.end(), '\\', '/');

    if(basePath[basePath.size() - 1] != '/')
    {
        basePath = basePath + "/";
    }

    //��ʼ����Դ��Ϣ��paths.xml���õ�·�����ó���Դ·����
    //��openSectionֻ�������·����
    char *sourcePath[2];
    sourcePath[0] = "-r";
    sourcePath[1] = (char *)basePath.c_str();
    if(!BWResource::init(2, sourcePath))
    {
        return  Script::getData(-2);
    }

    //����pythonģ��
    runInitTimeJobs();

    DataSectionPtr section = BWResource::openSection( "entities/entities.xml");
    if(!section)
    {
        INFO_MSG( "ERROR openSection entities.xml....\n" );
        return  Script::getData(-3);
    }

    g_pDatabases = new CXmlDatabase();

    g_pDatabases->init(section);

    return  Script::getData(0);
}

/**
 *  ReaMgr.finish ж��ģ��
 */
static PyObject *finishEntityDB(PyObject *self, PyObject *args)
{
    delete g_pDatabases;
    Script::fini(0);
    BWResource::instance().purgeAll();
    Py_Return;
}

static PyMethodDef ResMgrMethods[] = 
{
    {"init", loadEntityDB, METH_VARARGS, "����ģ��������Դ,һ����������ָ����Դ��ŵ�·��,\n\
        entities/entities.xml�������ָ����Ŀ¼"},
    {"finish", finishEntityDB, METH_NOARGS, "ж��ģ��������Դ"},
    { NULL, NULL}
};

PyMODINIT_FUNC initResMgr(void) 
{
    PyObject* m;

    m = Py_InitModule3("ResMgr", ResMgrMethods,
            "��Դ���.");

}
void ptest()
{
    PyObject* m =  Py_BuildValue("(s)","/home/hsq/love3/trunk/bigworld/src/server/tools/datatool/bin/res");

    loadEntityDB(NULL,m);
}
