/**
 *  @brief      ���ݶ��崦��
 *  @file       xmldefine.h
 *  @version    1.0.0
 *  @date       2011-01-29
 */
#ifndef XMLDEFINE_H
#define XMLDEFINE_H
#include <string>
#include <stack>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>
class CXmlDefine;
typedef std::vector < CXmlDefine * > VEC_CXMLDEFINE;
/**
 *  @class CXmlDefine 
 *  ʵ�嶨��˵��
 */
class CXmlDefine
{
    public:

        CXmlDefine();

        ~CXmlDefine();

        VEC_CXMLDEFINE &child() { return child_; };

        const std::string &name() { return name_; };
        void name(const std::string &value) { name_ = value; };

        const std::string &type() { return type_; };
        void type(const std::string &value) { type_ = value; };

        const std::string &defvalue() { return default_; };
        void defvalue(const std::string &value) { default_ = value; };


        int writeFile(std::ofstream &outfile, int n = 0);

        /** ��鶨�� */
        bool check(std::ifstream &stream);

        int childCount() { return child_.size(); }

    private:

        std::string name_;

        std::string type_;

        /** 
         *  ��������� SQUENCE ���ֵ��Ϊ�ձ�ʾ��
         *  <SK Type="SQUENCE"> INT32 </SK>
         */
        std::string default_;

        VEC_CXMLDEFINE child_;
};
#endif
