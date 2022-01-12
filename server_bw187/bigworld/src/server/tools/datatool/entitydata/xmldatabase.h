/**
 *  @brief      xml���ݿ�����ṩ��xml���ݿ�ĳ�ʼ����
 *              ��ѯ�����²�������
 *  @file       xmldatabase.h
 *  @version    1.0.0
 *  @date       2011-01-29
 */
#ifndef XMLDATABASE_H
#define XMLDATABASE_H
#include "xmltable.h"
#include "entitydefs.h"
typedef std::map< std::string, CXmlTable * > MAP_CXMLTABLE;

/**
 *  @class CXmlDatabase
 *  ���������ȫ��Ψһ
 */
class CXmlDatabase
{
    public:

        CXmlDatabase();

        ~CXmlDatabase();

        /** ��ʼ�� */
        int init(DataSectionPtr section);
        
        /** ��ȡ���� */
        DataSectionPtr openEntity(const std::string &name, int seq);

        /** ����Section���ڴ��¼ */
        int saveEntity(DataSectionPtr section, int seq);

        /** ȡ�����ݳ�Ϊ�ֵ� */
        PyObject *openData(const std::string &name, int seq);

        /** �����ֵ䵽�ڴ��¼ */
        int openData(const std::string &name, int seq, PyObject *dict);

        /** �������ݵ��ļ� */
        void save(const std::string &name);

        /** ����ʵ�嶨������ݵ����µĽṹ */
        int updateDefinData(const std::string &name);

        /** ɾ��һ������ */
        int deleteEntity(const std::string &name, int seq);

    private:

        EntityDefs *pEntityDefs_;

        MAP_CXMLTABLE databases_;
};
template <class DATATYPE>
void classTypeMapping( const std::string& propName, const DATATYPE& type, CXmlDefine *def = NULL);

void propertyMapping( const std::string& propName, const DataType& type, CXmlDefine *def = NULL);
#endif
