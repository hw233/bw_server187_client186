/**
 *  @brief      �������������
 *  @file       xmltable.cpp
 *  @version    1.0.0
 *  @date       2011-01-29
 */
#include "xmltable.h"
#include <algorithm>
#include "resmgr/bwresource.hpp"

/** ��󵥸���¼��С */
#define MAX_BUFFER 0x4000

/** ��¼��ʼ�ͽ���ָ�� */
typedef struct rowpos
{
    int begin;
    int end;
}ROWPOS;

CXmlTable::CXmlTable(const std::string &name):
    tableName_(name), 
    defineChange_(true), 
    dataChange_(true), 
    datas_(NULL),
    dataFile_(""),
    defineFile_(""),
    fileLength_(-1)
{
    dataFile_ = BWResource::getDefaultPath() + "/entities/datas" + "/" + tableName_ + ".xml";
}

CXmlTable::~CXmlTable()
{
    if(datas_ != NULL)
    {
        delete datas_;

        for(unsigned int i = 0; i < datas_->size(); i++)
        {
            delete datas_->at(i);
        }
    }

    if(dataFtream_.is_open())
        dataFtream_.close();
}

/** 
 *  �ȽϺͱ���Ķ����Ƿ���ı�
 */
bool CXmlTable::checkDefine()
{
    defineChange_ = false;

    std::ifstream stream;
    defineFile_ = BWResource::getDefaultPath() + "/entities/xmldef" + "/" + tableName_ + ".xml";
    stream.open (defineFile_.c_str(), std::ios::binary|std::ios::in);
    if(!stream.is_open())
    {
        printf("�ļ�������:%s\n", defineFile_.c_str());
        defineChange_ = true;
        return defineChange_;
    }

    char stringline[1024] = {};
    //<root>
    stream.getline(stringline, 1024);

    //while(!stream.eof()) 
    for(unsigned int i = 0; i < proprties_.size(); i++)
    {
        if(proprties_[i]->check(stream))
        {
            defineChange_ = true;
            break;
        }
    }
    //</root>
    stream.getline(stringline, 1024);

    return defineChange_;
}

/** 
 *  ����������������
 */
VEC_CDATANODE *CXmlTable::loadData()
{
    std::ifstream stream;
    struct stat fileInfo;
    char *tmpBuffer;
    int currentpos, count = 0, tab = 0, bpos = 0, n = 1;

    CDataNode *node = NULL, *lnode = NULL;
    std::stack< VEC_CDATANODE * > tmpstack;
    VEC_CDATANODE *pNode = new VEC_CDATANODE;
    VEC_CDATANODE *fnodes = pNode;
    tmpstack.push(fnodes);

    if(stat(dataFile_.c_str(), &fileInfo) != 0)
    {
        printf("�ļ�������:%s\n", dataFile_.c_str());
        return NULL;
    }

    tmpBuffer = new char[fileInfo.st_size]; 

    stream.open (dataFile_.c_str(), std::ios::binary|std::ios::in);

    stream.read((char *)tmpBuffer, (std::streamsize)fileInfo.st_size);

    stream.close();

    bpos = 0;

    //������һ��
    while(*(tmpBuffer+bpos) != 0x0A) bpos++;

    for(currentpos = bpos ; currentpos < fileInfo.st_size;)
    {
        if(tmpBuffer[currentpos] == 0x0A)
        {
            count++;
            bpos = currentpos + 1;
            tab = 0;
            currentpos++;
        }
        else if(tmpBuffer[currentpos] == '	')
        {
            tab++;
            currentpos++;
        }
        else if(bpos + tab == currentpos)
        {
            if(tab == 0)
                break;

            if(tab > n)
            {
                fnodes = &lnode->child();
                tmpstack.push(fnodes);
                n = tab;
            }
            else if( tab < n)
            {
                tmpstack.pop();
                fnodes = tmpstack.top();
                n = tab;
            }

            if(tmpBuffer[currentpos + 1] != '/')
            {
                node = new CDataNode();
                int len = node->init((const char *)(tmpBuffer + currentpos), tab);
                fnodes->push_back(node);
                lnode = node;
                currentpos = currentpos + len;
            }
            else
            {
                currentpos++;
            }

        }
        else
        {
            currentpos++;
        }
    }

    delete tmpBuffer;

    return pNode;
}

/** 
 *  �������ݼ�¼���ļ�ƫ�������Ա���ٷ��� 
 */
int CXmlTable::loadDataIndex()
{
    std::ifstream stream;
    struct stat fileInfo;
    char *tmpBuffer;
    int currentpos, bpos = 0;

    datas_ = new VEC_CDATANODE;

    if(stat(dataFile_.c_str(), &fileInfo) != 0)
    {
        printf("�ļ�������:%s\n", dataFile_.c_str());
        return -1;
    }

    fileLength_ = fileInfo.st_size;

    tmpBuffer = new char[fileInfo.st_size]; 

    dataFtream_.open (dataFile_.c_str(), std::ios::binary|std::ios::in);
    if(!dataFtream_.is_open())
    {
        printf("�ļ�������:%s\n", dataFile_.c_str());
        return -1;
    }

    dataFtream_.read((char *)tmpBuffer, (std::streamsize)fileInfo.st_size);


    bpos = 0;

    //������һ��
    while(*(tmpBuffer+bpos) != 0x0A) bpos++;

    CDataNode *bn = NULL;
    for(currentpos = bpos ; currentpos < fileInfo.st_size;)
    {
        if(tmpBuffer[currentpos] == 0x0A && 
                tmpBuffer[currentpos + 1] == 0x09 && 
                tmpBuffer[currentpos + 2] == 0x3C &&
                tmpBuffer[currentpos + 3] == 0x72 && 
                tmpBuffer[currentpos + 4] == 0x6F &&
                tmpBuffer[currentpos + 5] == 0x77 &&
                tmpBuffer[currentpos + 6] == 0x3E)
        {
            int addr = currentpos + 1;
            CDataNode *node = new CDataNode("row", std::string((char *)&addr, 4), 1);
            datas_->push_back(node);
            currentpos = currentpos + 7;
            bn = node;
        }
        else if(tmpBuffer[currentpos] == 0x0A && 
                tmpBuffer[currentpos + 1] == 0x09 && 
                tmpBuffer[currentpos + 2] == 0x3C &&
                tmpBuffer[currentpos + 3] == 0x2F &&
                tmpBuffer[currentpos + 4] == 0x72 &&
                tmpBuffer[currentpos + 5] == 0x6f &&
                tmpBuffer[currentpos + 6] == 0x77 &&
                tmpBuffer[currentpos + 7] == 0x3E)
        {
            int addr = currentpos + 10;
            //д��¼��βƫ����
            bn->value().append((char *)&addr, 4);
            currentpos = currentpos + 7;
        }
        else
        {
            currentpos++;
        }
    }

    delete tmpBuffer;

    printf("Info [%s] record [%d]...\n", tableName_.c_str(), datas_->size());

    return datas_->size();

}
/**
 *  ��ȡ����
 */
DataSectionPtr CXmlTable::makeDataSection(int seq)
{
    DataSectionPtr ds= new XMLSection(tableName_);

    if((int)datas_->size() <= seq)
    {
        printf("Error number ...\n");

        return ds;
    }

    CDataNode *c = datas_->at(seq);

    //�ڵ�ֵ���汣���������������ƫ����
    if(c->value().size() != 0)
    {
        ROWPOS *pos = (ROWPOS *)c->value().c_str();
        c = rowData(c, pos->begin, pos->end);
    }

    for(int i = 0; i < c->childCount(); i++)
    {
        c->child(i)->makeDataSection(ds);
    }

    return ds;
}
/** 
 *  ���ص������� 
 *  @param row  ���ڵ�
 *  @param posBegin ��¼���ļ��Ŀ�ʼƫ����
 *  @param posEnd   ��¼���ļ��Ľ�βƫ����
 *                  -1��ʾ�ļ���β   
 */
CDataNode *CXmlTable::rowData(CDataNode *row, int posBegin, int posEnd)
{
    char *tmpBuffer;
    int currentpos, count = 0, tab = 0, bpos = 0, n = 2, bufLen;

    CDataNode *node = NULL, *lnode = NULL;
    std::stack< VEC_CDATANODE * > tmpstack;
    VEC_CDATANODE *fnodes = &row->child();
    tmpstack.push(fnodes);

    bufLen = posEnd - posBegin - 8/*</row>*/;

    tmpBuffer = new char[bufLen]; 

    dataFtream_.seekg(posBegin);
    dataFtream_.read((char *)tmpBuffer, (std::streamsize)bufLen);


    bpos = 0;

    //������һ��<row>
    while(*(tmpBuffer+bpos) != 0x0A) bpos++;

    for(currentpos = bpos ; currentpos < bufLen;)
    {
        if(tmpBuffer[currentpos] == 0x0A)
        {
            count++;
            bpos = currentpos + 1;
            tab = 0;
            currentpos++;
        }
        else if(tmpBuffer[currentpos] == '	')
        {
            tab++;
            currentpos++;
        }
        else if(bpos + tab == currentpos)
        {
            if(tab == 0)
                break;

            if(tab > n)
            {
                fnodes = &lnode->child();
                tmpstack.push(fnodes);
                n = tab;
            }
            else if( tab < n)
            {
                tmpstack.pop();
                fnodes = tmpstack.top();
                n = tab;
            }

            if(tmpBuffer[currentpos + 1] != '/')
            {
                node = new CDataNode();
                int len = node->init((const char *)(tmpBuffer + currentpos), tab);
                fnodes->push_back(node);
                lnode = node;
                currentpos = currentpos + len;
            }
            else
            {
                currentpos++;
            }

        }
        else
        {
            currentpos++;
        }
    }

    delete tmpBuffer;

    //row->value("");
    return row;
}

/** 
 *  �����¶�������ݽṹ�������� 
 */
int CXmlTable::alterDataStruct()
{

    if(datas_ != NULL)
    {
        printf("define no change...\n");
        return -1;
    }
    else
    {
        //���ص��ڴ�
        VEC_CDATANODE *pNode = loadData();

        if(pNode == NULL)
        {
            std::ofstream outfile(dataFile_.c_str(), std::ofstream::binary);
            std::string emptyFile = "<root>\r\n</root>";
            outfile.write(emptyFile.c_str(), emptyFile.size());
            outfile.close();
        }
        else
        {
            //����
            for(unsigned int i = 0; i < pNode->size(); i++)
            {
                CDataNode *p = pNode->at(i);
                pNode->at(i) = verifyData(p);
                //row
            }

            //��������
            saveDatas(pNode);

            delete pNode;
        }

        //���涨��
        saveDefine();

        loadDataIndex();
    }

    return 0;
}

/** 
 *  �����ݵ����� 
 */
int CXmlTable::seqVerifyData(CXmlDefine *def, CDataNode *oRow, CDataNode *pRow)
{
    if(def->defvalue() == "FIXEDDICT")
    {
        for(int j = 0; j < oRow->childCount(); j++)
        {
            CDataNode *row = oRow->child(j);
            CDataNode *nRow = new CDataNode(row->name(), row->value(), row->layer());
            dictVerifyData(def, row, nRow);
            pRow->addChildNode(nRow);
        }
    }
    else
    {
        for(int j = 0; j < oRow->childCount(); j++)
        {
            CDataNode *row = oRow->child(j);
            CDataNode *nRow = new CDataNode(row->name(), row->value(), row->layer());
            pRow->addChildNode(nRow);
        }
    }
    return 0;
}

int CXmlTable::dictVerifyData(CXmlDefine *def, CDataNode *oRow, CDataNode *pRow)
{
    MAP_CDATANODE mapChild;
    for(int j = 0; j < oRow->childCount(); j++)
    {
        //ÿ������
        CDataNode *pd = oRow->child(j);
        mapChild.insert(MAP_CDATANODE::value_type(pd->name(), pd));
    }

    VEC_CXMLDEFINE &child = def->child();

    for(unsigned int i = 0; i < child.size(); i++)
    {
        std::string name = child[i]->name();
        MAP_CDATANODE::iterator itor = mapChild.find(name);

        if(itor != mapChild.end())
        {
            CDataNode *cRow = new CDataNode(itor->second->name(), 
                    itor->second->value(), itor->second->layer());

            if(child[i]->type() == "SEQUENC")
            {
                seqVerifyData(child[i], itor->second, cRow);
            }
            if(child[i]->type() == "FIXEDDICT")
            {
                dictVerifyData(child[i], itor->second, cRow);
            }

            pRow->addChildNode(cRow);
        }
        else
        {
            CDataNode *cRow = new CDataNode(child[i]->name(), "", oRow->layer() + 1);
            pRow->addChildNode(cRow);
        }

    }

    return 0;
}

/** 
 *  �������ݵ����� 
 *  @param row Ҫ���������
 */
CDataNode *CXmlTable::verifyData(CDataNode *row)
{
    // �������ӽڵ�洢��������item�ڵ㡣ʹ������ȡֵʱʹ�� 
    CDataNode *nRow = new CDataNode(row->name(), row->value(), row->layer());
    MAP_CDATANODE mapChild;
    for(int j = 0; j < row->childCount(); j++)
    {
        //ÿ������
        CDataNode *pd = row->child(j);
        mapChild.insert(MAP_CDATANODE::value_type(pd->name(), pd));
    }

    for(unsigned int i = 0; i < proprties_.size(); i++)
    {
        std::string name = proprties_[i]->name();
        MAP_CDATANODE::iterator itor = mapChild.find(name);

        if(itor != mapChild.end())
        {
            CDataNode *cRow = new CDataNode(itor->second->name(), 
                    itor->second->value(), itor->second->layer());
            if(proprties_[i]->type() == "SEQUENC")
            {
                seqVerifyData(proprties_[i], itor->second, cRow);
            }
            if(proprties_[i]->type() == "FIXEDDICT")
            {
                dictVerifyData(proprties_[i], itor->second, cRow);
            }

            nRow->addChildNode(cRow);
        }
        else
        {
            CDataNode *cRow = new CDataNode(proprties_[i]->name(), "", row->layer() + 1);
            nRow->addChildNode(cRow);
        }
    }

    delete row;

    return nRow;
}

/** 
 *  ���涨�� 
 */
int CXmlTable::saveDefine()
{
    std::ofstream outfile(defineFile_.c_str(), std::ofstream::binary);

    outfile.write("<root>\n", 7);
    for(unsigned int i = 0; i < proprties_.size(); i++)
    {
        proprties_[i]->writeFile(outfile, 1);
    }
    outfile.write("</root>\n", 8);
    outfile.close();
    return 0;
}

/** 
 *  �������� 
 */
int CXmlTable::saveDatas(VEC_CDATANODE *pNode)
{
    VEC_CDATANODE *node = NULL;
    if(pNode == NULL)
        node = datas_;
    else
        node = pNode;

    if(node == NULL)
    {
        printf("Error no data...\n");
        return -1;
    }

    char *tmpBuffer = new char[MAX_BUFFER]; 

    std::ofstream outfile(std::string(dataFile_ + ".bak").c_str(), std::ofstream::binary);

    outfile.write("<root>\r\n", 8);
    for(unsigned int i = 0; i < node->size(); i++)
    {
        if(node->at(i)->value().size() != 0 && node->at(i)->childCount() <= 0)
        {
            //д�����ļ�û�м��ص��ڴ������
            ROWPOS *pos = (ROWPOS *)(node->at(i)->value().c_str());
            int len = pos->end - pos->begin; 
            dataFtream_.seekg(pos->begin);
            dataFtream_.read((char *)tmpBuffer, len);
            outfile.write(tmpBuffer, len);
        }
        else
        {
            node->at(i)->value("");
            node->at(i)->writeFile(outfile);
            outfile.write("\r\n", 2);
        }
    }

    outfile.write("</root>\r\n", 9);
    outfile.close();

    delete tmpBuffer;

    return 0;
}

/** 
 *  DataSectionת���CDataNode 
 */
int CXmlTable::sectionToDataNode(DataSectionPtr ds, int seq)
{
    if(datas_ == NULL)
    {
        printf("Error no data...\n");
        return -1;
    }

    CDataNode *node = new CDataNode(datas_->at(seq)->name(), 
            datas_->at(seq)->value(), 
            datas_->at(seq)->layer());

    node->sectionToDataNode(ds);
    if(seq == -1)
    {
        datas_->push_back(node);
        seq = datas_->size();
    }
    else
    {
        delete datas_->at(seq);
        datas_->at(seq) = node;
    }

    return seq;
}

/** 
 *  ɾ��һ������ 
 */
int CXmlTable::deleteEntity(int seq)
{
    if(datas_ != NULL)
    {
        printf("Error no data...\n");
        return -1;
    }

    if((int)datas_->size() < seq)
    {
        printf("Error number[%d][%d] ...\n", datas_->size(), seq);
        return -1;
    }

    datas_->erase(datas_->begin() + seq);

    return 0;
}
