/**
 *  @brief      ����xml�ļ�����
 *  @file       xmldatas.h
 *  @version    1.0.0
 *  @date       2011-01-29
 */
#ifndef XMLDATAS_H
#define XMLDATAS_H
#include <string>
#include <stack>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
#include "cstdmf/debug.hpp"
#include "pyscript/script.hpp"
#include "pyscript/py_data_section.hpp"
#include "server/bwconfig.hpp"
#include "resmgr/xml_section.hpp"
#include "entitydef/data_types.hpp"
#include "resmgr/xml_section.hpp"
#include "xmldefine.h"
class CDataNode;

typedef std::vector < CDataNode * > VEC_CDATANODE;
typedef std::map< std::string, CDataNode * > MAP_CDATANODE;

inline int numbTab(const char *buf)
{
    const char *p = buf;
    while(*p == '	')p++;
    return (int)(p - buf);
}

/**
 *  @class CDataNode
 *  ����ÿ���ڵ�����ƺ�ֵ���ͶԽڵ�Ĳ�����
 */
class CDataNode
{
    public:
        CDataNode();
        CDataNode(const char *buff, int layer);
        CDataNode(const std::string &name, const std::string &value, int layer);

        ~CDataNode();

        int init(const char *buff, int layer);

        void value(const std::string &v);
        std::string &value() { return value_; }

        VEC_CDATANODE &child()
        { 
            if(child_ == NULL)
                child_ = new VEC_CDATANODE;
            return *child_; 
        };

        int childCount() 
        { 
            if(child_ == NULL)
                return -1;
            return child_->size(); 
        }

        CDataNode *child(int i) 
        { 
            if(child_ == NULL)
                return NULL;
            return child_->at(i); 
        }

        int addChildNode(CDataNode *node);

        int delChildNode(unsigned int index);

        int writeFile(std::ofstream &outfile);

        std::string &name() { return name_; }

        int layer() { return layer_; }

        /** ת���DataSection */
        int makeDataSection(DataSectionPtr ds);
        
        /** DataSectionת���CDataNode */
        int sectionToDataNode(DataSectionPtr ds);

    private:
        std::string name_;

        std::string value_;

        /** �ӽڵ� */
        VEC_CDATANODE *child_;

        /** ��� */
        int layer_;

        /** �����ĸ����� */
        CXmlDefine *pBelong_;
};
#endif
