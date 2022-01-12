/**
 *  @brief      �������������
 *  @file       xmltable.h
 *  @version    1.0.0
 *  @date       2011-01-29
 */
#ifndef XMLTABLE_H
#define XMLTABLE_H
#include "xmldatas.h"
#include "xmldefine.h"
class CXmlTable
{
    public:

        CXmlTable(const std::string &name);
        ~CXmlTable();

        bool change() { return dataChange_; };

        void change(bool value) { dataChange_ = value; };

        VEC_CXMLDEFINE &proprties() { return proprties_; };

        /** �ȽϺͱ���Ķ����Ƿ�ı� */
        bool checkDefine();

        /** �������ݣ��ڸ������ݽṹʹ�� */
        VEC_CDATANODE *loadData();


        /** �������ݼ�¼���ļ�ƫ�������Ա���ٷ��� */
        int loadDataIndex();

        /** ��ȡ��¼ */
        DataSectionPtr makeDataSection(int seq);

        /** DataSectionת���CDataNode */
        int sectionToDataNode(DataSectionPtr ds, int seq);

        /** �����¶�������ݽṹ�������� */
        int alterDataStruct();

        /** �������ݵ����� */
        CDataNode *verifyData(CDataNode *row);

        /** �����ݵ����� */
        int seqVerifyData(CXmlDefine *def, CDataNode *oRow, CDataNode *pRow);
        int dictVerifyData(CXmlDefine *def, CDataNode *oRow, CDataNode *pRow);

        /** ���涨�� */
        int saveDefine();

        /** �������� */
        int saveDatas(VEC_CDATANODE *pNode = NULL);

        /** ɾ��һ������ */
        int deleteEntity(int seq);

        const std::string &tableName() { return tableName_; }

    private:

        /** ���ص������� */
        CDataNode *rowData(CDataNode *row, int posBegin, int posEnd);

        /** ʵ������ */
        std::string tableName_;

        /** �����ı�� */
        bool defineChange_;

        /** ���ݷ�ı�� */
        bool dataChange_;

        /** ���� */
        VEC_CDATANODE *datas_;

        /** �����ļ�·�� */
        std::string dataFile_;

        /** �����ļ�·�� */
        std::string defineFile_;

        /** ���� */
        VEC_CXMLDEFINE proprties_;

        /** �򿪵������ļ� */
        std::ifstream dataFtream_;

        /** �����ļ����� */
        int fileLength_;
};
#endif
