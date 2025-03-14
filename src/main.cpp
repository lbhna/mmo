

#include "mmo_lib.h"
#include <string>
#include <stdio.h>
#include <cstring> 
#include <iostream>
#include <cstdint>


class Point2D
{
public:
  Point2D(){}
  Point2D(int32_t a,int32_t b){x=a;y=b;}
public:
  int32_t   x{0};
  int32_t   y{0};
};
typedef mmo::offset_ptr<mmo::string<int16_t>,int16_t> PString;
typedef mmo::hash_map<int32_t,PString,int16_t> LabelMap;
class CRoad
{
protected:
  uint64_t                         m_id{0};
  mmo::string<int16_t>             m_name; 
  mmo::vector<Point2D,int16_t>     m_coors;
  LabelMap                         m_labels;
public:
  CRoad()
  {
  }
public:
  uint64_t        get_id()const{return m_id;}
  const char*     get_name()const{return m_name.c_str();}
  int32_t         get_coors_size()const{return m_coors.size();}
  const Point2D&  get_coors(int32_t index)const {return m_coors[index];}
public:
  void init(uint64_t id,const std::string& name,const std::vector<Point2D>& coors,mmo::segment_manager& segment)
  {
    m_id = id;
    m_name.assign(name,segment);
    m_coors.assign(coors,segment);

    int count = id+1;

    m_labels.init_hash(count,segment);    
    for(int i=0;i<count;i++)
    {
      PString ss = mmo::construct<mmo::string<int16_t>>(segment);
      ss->assign(std::to_string(id)+"_label_"+std::to_string(i+1),segment);       
      m_labels.add(i+100,ss,segment);
    }
  }

  void print()
  {
    printf("id= %ld\r\nname=%s\r\n",get_id(),get_name());
    for(int i=0;i<get_coors_size();i++)
    {
      const Point2D&  coord = get_coors(i);
      printf("coor_%d = [%d,%d]\r\n",i,coord.x,coord.y);
    }

    for(auto it=m_labels.begin();it!=m_labels.end();++it)
    {
      printf("label = {%d , %s}\r\n",it.node->key,(*it)->c_str());
    }
  }
  void show_label(int32_t label_id)
  {
    printf("show_label[%d] : %s\r\n",
      label_id,
      m_labels[label_id]->c_str());
  }
};

class CRoadMap
{
protected:
  int m_count{0};
  mmo::var_vector<CRoad,int16_t>  m_road_map;
public:
  CRoadMap(){}
  void init(int count,mmo::segment_manager& segment)
  {
    m_count=count;
    m_road_map.prepare_append_elements(segment);
    for (int i = 0; i < m_count; i++)
    {
      auto element = m_road_map.begin_append_element(segment);

      std::vector<Point2D> coors;
      for(int j=0;j<2+i;j++)
        coors.emplace_back(10*(i+1)+j,20*(i+1)+j);

      element->object().init(i+1,"road_"+std::to_string(i+1),coors,segment);

      m_road_map.end_append_element(element,segment);
    }
  }
  void print()
  {
    printf("road_count=%d\n",m_count);
    
    for(auto it = m_road_map.begin();it!=m_road_map.end();++it)
    {
      printf("=====================\r\n");

      (*it).print();

      (*it).show_label(101);

    }
  }

};

void save()
{
  //预分配内存
  char buf[2048];
  mmo::segment_manager segment(buf,sizeof(buf));
 
  //构造RoadMap
  CRoadMap* pRoadMap = mmo::construct<CRoadMap>(segment);
  //构造3个对象
  pRoadMap->init(3,segment);
  
  //调用 对象的方法，输出构造的信息，用于后续的验证
  pRoadMap->print();

  //保存到文件
  FILE* fp = fopen("1.dat","wb");
  if(fp)
  {
    fwrite(segment.data(),segment.size(),1,fp);
    fclose(fp);
  }  
}

void load()
{
  //读取文件内容
  FILE* fp = fopen("1.dat","rb");
  if(fp==nullptr)
    return;    
  char buf[1024];
  fread(buf,sizeof(buf),1,fp);
  fclose(fp);

  //无需进行数据到对象的序列化操作，可直接映射成对象使用
  CRoadMap* pRoadMap = (CRoadMap*)buf;

  //调用 映射的对象的方法，验证其成员函数获取信息的正确性
  pRoadMap->print();

}

int main(int argc, char* argv[]) 
{
    // 检查参数数量是否为 2（程序名 + 一个参数）
    if (argc != 2) {
        std::cerr << "用法: " << argv[0] << " [save|load]" << std::endl;
        return 1;
    }

    // 根据参数调用对应函数
    if (strcmp(argv[1], "save") == 0) 
    {
        save();
    } 
    else if (strcmp(argv[1], "load") == 0) 
    {
        load();
    } else {
        std::cerr << "错误: 无效参数，请输入 'save' 或 'load'" << std::endl;
        std::cerr << "用法: " << argv[0] << " [save|load]" << std::endl;
        return 1;
    }

    return 0;
}