#pragma once

/*************************************************\
* @file   : mmo_lib.h
* @author : lb108197
* @contact: lb108197@alibaba-inc.com
* @brief  : Memory Map Objects library 
*           复杂对象--线性映射库--无内存分配
* @version: 1.1
* @date   : 2020/11/30
\*************************************************/
#include <string>
#include <vector>
#include <list>
#include <exception>
#include <string.h>
#include <stdio.h>
namespace mmo
{

/**
 * @brief 内存映射对象异常类
 * 
 */
class mmo_exception:
  public std::exception
{
public:
  enum 
  {
    ok               = 0,
    no_enough_memory = 1001,
    invalid_memory_address = 1002,
    unknown_exception= 9999,
  };
protected:
  std::string       m_msg;
  int32_t           m_code{0};
public:
  mmo_exception(int32_t code,const std::string& msg)_GLIBCXX_USE_NOEXCEPT
  {
    m_msg   = msg;
    m_code  = code;
  }
public:
  const std::string&  msg()   const _GLIBCXX_USE_NOEXCEPT {return m_msg;}
  int32_t             code()  const _GLIBCXX_USE_NOEXCEPT {return m_code;}
  virtual const char* what()  const _GLIBCXX_USE_NOEXCEPT
  {
    return m_msg.c_str();
  }
  
};

/**
 * @brief 内存映射对象内存段管理器
 * 
 */
class segment_manager
{
protected:
  char*       m_buffer{nullptr};
  size_t      m_capacity{0};
  char*       m_current{nullptr};
  char*       m_end{nullptr};
public:
  segment_manager(){}
  segment_manager(char* buffer,size_t capacity){reset(buffer,capacity);}
  ~segment_manager() {}
public:
  void    reset(char* buffer,size_t capacity)
  {
    m_capacity  = capacity;
    m_buffer    = buffer;
    m_current   = m_buffer;
    m_end       = m_buffer + m_capacity;
  }
public:
  char*     alloc(size_t size)
  {
    char* p = m_current;
    if( (m_current + size) > m_end)
      return NULL;
    m_current += size;
    return p;
  }
  size_t    calc_offset(void* obj_addr){return (m_current - (char*)obj_addr); }  
  bool      enough(size_t size)const{ return ( (m_current + size) <= m_end); }
  char*     current()const{return m_current;}
  size_t    capacity()const{return m_capacity;}
  bool      advance(size_t size)
  {
    if(!enough(size))
      return false;
    m_current += size;
    return true;
  }
  size_t    get_free_memory()const{return (m_end-m_current);}
  size_t    size()const{return (m_current-m_buffer);}
  const char* data()const{return m_buffer;}
  bool       verify_addr(void* addr)
  {
    return (addr >= m_buffer && addr < m_end);
  }
  void      exception_addr(void* addr)
  {
    if(addr < m_buffer || addr >= m_end)
      throw mmo_exception((int32_t)mmo_exception::invalid_memory_address,"mmo_exception:: invalid memory address!");
  }
};

/**
 * @brief 无内存分配 ，线性空间，对象构造器函数
 * 
 * @tparam T 
 * @param segment 
 * @return T* 
 */
template<typename T> 
T* construct(segment_manager& segment)
{
  void* p = segment.alloc(sizeof(T));
  if(p == NULL)
  {
    size_t free_size = segment.get_free_memory();
    size_t used_size = segment.size();
    size_t new_size = sizeof(T);
    std::string strErrMsg = "mmo_exception:: no enough memory,free:" + std::to_string(free_size) + ",used:"
        + std::to_string(used_size) + ",alloc size:" + std::to_string(new_size) ;
    throw mmo_exception((int32_t)mmo_exception::no_enough_memory,strErrMsg);
    return NULL;
  }
  ::new(p)T(); 
  return (T*)p;
}

//====================================================================================================
#pragma pack(push,1)

/**
 * @brief 相对寻址指针模板类
 * 
 * @tparam ValueType 
 * @tparam OffsetType 注意：类型最大值 需大于 寻址空间最大值 
 */
template<typename ValueType,typename OffsetType>
class offset_ptr
{
  typedef       offset_ptr<ValueType,OffsetType>    SelfType;
  typedef       std::size_t                         AddrType;
protected:
  OffsetType    m_offset;
public:
  offset_ptr()
  {
    m_offset = raw_to_offset((AddrType)this,(AddrType)0);
  }
  offset_ptr(const ValueType* ptr)
  {
    m_offset = raw_to_offset((AddrType)this,(AddrType)ptr);
  }
  offset_ptr(const SelfType& other)
  {
    m_offset = raw_to_offset((AddrType)this,(AddrType)other.get());
  }
  ValueType* get()
  {  
    return (ValueType*)offset_to_raw((AddrType)this,m_offset);   
  }
  const ValueType* get()const
  {  
    return (const ValueType*)offset_to_raw((AddrType)this,m_offset);   
  }
  ValueType* operator->()
  {  
    return this->get(); 
  }
  const ValueType* operator->()const
  {  
    return this->get(); 
  }
  ValueType& operator* ()
  {
    return *this->get();
  }
  const ValueType& operator* ()const
  {
    return *this->get();
  }
  SelfType& operator= (ValueType* from)
  {
    m_offset = raw_to_offset((AddrType)this,(AddrType)from);
    return *this;
  }

  SelfType& operator= (const SelfType& other)
  {
    m_offset = raw_to_offset((AddrType)this,(AddrType)other.get());
    return *this;
  }
  
  bool operator== (const SelfType& other)const
  {
    return get() == other.get();
  } 
  bool operator!= (const SelfType& other)const
  {
    return get() != other.get();
  }  
  bool operator==(const ValueType* other)const
  {
    return get() == other;
  }
  bool operator!=(const ValueType* other)const
  {
    return get() != other;
  }
protected:
  /**
   * @brief 偏移地址向原始地址转换
   * 
   * @param base 
   * @param offset 
   * @return AddrType 
   */
  inline AddrType offset_to_raw(AddrType base,OffsetType offset)const
  {
    return AddrType(base + offset) & -AddrType(offset != 1);
  }
  /**
   * @brief 原始地址到偏移量的转换，与-1 相当于不变，与-0 相当于清零，当原地址是0时，结果为1，因此offset=1表示是空指针
   *        为什么用1 ？实在是其它值不合适，0是用来指向自己的，只有1是指向自己的第二个字节，理论上不会有这种指向。
   * @param base 
   * @param raw 
   * @return OffsetType 
   */
  inline OffsetType raw_to_offset(AddrType base,AddrType raw)const
  {
    return OffsetType(( AddrType(raw - base - 1) & -AddrType(raw != 0) ) + 1);
  }
};

/**
 * @brief 无内存分配，内容相对地址存储，托管对象定长的vector模板类
 * 
 * @tparam ValueType 
 * @tparam SizeType 
 */
template<typename ValueType,typename SizeType>
class vector
{
  typedef vector<ValueType,SizeType>  SelfType;
protected:
  SizeType        m_size;
  SizeType        m_offset;
public:
  vector()
  {
    m_size    = 0;
    m_offset  = 0;
  }
  vector(const SelfType&) = delete; 
  SelfType& operator=(const SelfType&) = delete;
public:
  SizeType          _total_bytes()const{return sizeof(SelfType) + _data_bytes();}
  SizeType          _data_bytes()const{return (m_size * sizeof(ValueType));}
  void              _set_offset(SizeType offset){m_offset = offset;}
public:
  void              to_std(std::vector<ValueType>& dst)const
  {
    dst.resize(size());
    for(SizeType i= 0;i<size();i++)
      dst[i] = at(i);
  }
public:
  bool              resize(SizeType size,segment_manager& segment)
  {
    m_size    = size;
    m_offset  = (SizeType)segment.calc_offset(this);
    if(!segment.advance(_data_bytes()))
    {
      size_t free_size = segment.get_free_memory();
      size_t used_size = segment.size();
      size_t new_size = _data_bytes();
      std::string strErrMsg = "mmo_exception:: no enough memory,free:" + std::to_string(free_size) + ",used:"
        + std::to_string(used_size) + ",alloc size:" + std::to_string(new_size) ;
      throw mmo_exception((int32_t)mmo_exception::no_enough_memory,strErrMsg);
      return false;
    }
    
    ValueType* p=data();
    for(SizeType i=0;i<m_size;i++)
      ::new((void*)(p+i))ValueType();
    return true;
  }
  bool              assign(const std::vector<ValueType>& src,segment_manager& segment)
  {
    if(!resize(src.size(),segment))
      return false;
    SizeType i=0;
    for(auto& it:src)
      data()[i++] = it;
    return true;
  }
  bool              assign(const std::list<ValueType>& src,segment_manager& segment)
  {
    if(!resize(src.size(),segment))
      return false;
    SizeType i=0;
    for(auto& it:src)
      data()[i++] = it;
    return true;
  }  
  SizeType          size()const{ return m_size;}
  bool              empty()const{return m_size==0;}
public:
  ValueType&        operator[](SizeType index) {return data()[index];}
  const ValueType&  operator[](SizeType index)const {return data()[index];}
public:
  ValueType&        at(SizeType index) {return data()[index];}
  const ValueType&  at(SizeType index)const {return data()[index];}
public:
  ValueType*        data()  {return _get_data_addr();}  
  const ValueType*  data()const {return _get_data_addr(); }  
public:
  ValueType&        front(){return data()[0];}
  const ValueType&  front()const {return data()[0];}
  ValueType&        back(){return data()[m_size-1];}
  const ValueType&  back()const {return data()[m_size-1];}

protected:
  ValueType*        _get_data_addr(){return (ValueType*)((char*)this + m_offset); }  
  const ValueType*  _get_data_addr()const{return (const ValueType*)((char*)this + m_offset); }  
};


/**
 * @brief 无内存分配 ，内容相对地址存储，字串模板类
 * 
 * @tparam SizeType 
 */
template<typename SizeType>
class string
{
  typedef string<SizeType>  SelfType;
protected:
  SizeType        m_size{0};
  SizeType        m_offset{0};
public:
  string()
  {
    m_size    =0;
    m_offset  =0; 
  }
  string(const SelfType&) = delete; 
  SelfType& operator=(const SelfType&) = delete;
public:
  SizeType          _total_bytes()const
  {
    return sizeof(SelfType) + _data_bytes();
  }
  SizeType          _data_bytes()const
  {
    return (m_size != 0 ) ? ( (m_size+1) * sizeof(char)) : 0;
  }
  void              _set_offset(SizeType offset)
  {
    m_offset = offset;
  }
public:
  void  to_std(std::string& dst)const
  {
    dst.assign(data(),size());
  }
public:
  bool  assign(const std::string& src,segment_manager& segment)
  {
    m_size   = src.size();
    if(m_size == 0)
      return true;
    m_offset = segment.calc_offset(this);
    char* dst = segment.alloc(m_size+1);
    if(dst == nullptr)
    {
      m_size = 0;
      size_t free_size = segment.get_free_memory();
      size_t used_size = segment.size();
      size_t new_size = m_size+1;
      std::string strErrMsg = "mmo_exception:: no enough memory,free:" + std::to_string(free_size) + ",used:"
        + std::to_string(used_size) + ",alloc size:" + std::to_string(new_size) ;
      throw mmo_exception((int32_t)mmo_exception::no_enough_memory,strErrMsg);
      return false;
    }

    memcpy(dst , src.data() , m_size );
    dst[m_size]=0;  

    return true;
  }
public:
  SizeType          size()const{ return m_size;}
  SizeType          length()const {return m_size;}
  bool              empty()const{return m_size==0;}
public:
  char&             operator[](SizeType index) {return data()[index];}
  const char&       operator[](SizeType index)const {return data()[index];}
public:
  char*             data()  {return _get_data_addr();}  
  const char*       data()const {return _get_data_addr(); }  
  char*             c_str()  {return _get_data_addr();}  
  const char*       c_str()const {return _get_data_addr(); } 
protected:
  char*             _get_data_addr()
  {
    return (char*)this + m_offset;
  }
  const char*       _get_data_addr()const
  {
    return (const char*)this + m_offset;
  }    
};



/**
 * @brief 无内存分配 ，内容相对地址存储，托管对象变长的vector模板类
 * 
 * @tparam ValueType 
 * @tparam SizeType 
 */
template<typename ValueType,typename SizeType>
class var_element
{
  typedef var_element<ValueType,SizeType>  SelfType;
protected:
  SizeType        m_bytes;
public:
  var_element()
  {
    m_bytes = 0;
  }
public:
  SizeType          _total_bytes()const
  {
    return sizeof(SelfType) + _data_bytes();
  }
  SizeType          _data_bytes()const
  {
    return m_bytes;
  }
  void              _set_data(void* data,SizeType bytes)
  {
    memcpy(_get_data_addr(),data,bytes);
    m_bytes = bytes;
  }
  void              _set_data_size(SizeType bytes)
  {
    m_bytes = bytes;
  }
  char*             _get_data_addr()
  {
    return ((char*)this + sizeof(SelfType));
  } 
  const char*       _get_data_addr()const
  {
    return ((char*)this + sizeof(SelfType));
  } 
public:
  ValueType&        object(){return *(ValueType*)data();}
  const ValueType&  object()const{return *(ValueType*)data();}

  char*             data()  {return _get_data_addr();}  
  const char*       data()const {return _get_data_addr(); }


};

template<typename ValueType,typename SizeType>
class var_vector
{
  typedef var_element<ValueType,SizeType>   ElementType;
  typedef var_vector<ValueType,SizeType>    SelfType;
public:
  class _iterator
  {
  public:
    ElementType*    element_{NULL};
    SizeType        index_{0};
    SizeType        size_{0};
  public:
    _iterator(ElementType* element,SizeType index,SizeType size){element_  =element;index_    =index;size_     =size;}
    _iterator(const _iterator& rhs){element_  =rhs.element_;index_    =rhs.index_;size_     =rhs.size_;}
    bool operator==(const _iterator& _rhs) const{return (element_ == _rhs.element_ && index_ == _rhs.index_ && size_ == _rhs.size_);}
    bool operator!=(const _iterator& _rhs) const{return (element_ != _rhs.element_ || index_ != _rhs.index_ || size_ != _rhs.size_);}
    ValueType* operator->() const{return &element_->object();}
    ValueType& operator*() const{return element_->object();}
    _iterator operator++(int)
    {
      iterator _Tmp = *this;
      ++*this;
      return (_Tmp);
    }
    _iterator& operator++()
    {	
      if(element_ != NULL)
      {
        if(++index_ == size_)
        {
          element_ = NULL;
          return *this;
        }
        element_ = (ElementType*) ( (char*)element_ + element_->_total_bytes() );
      }
      return *this;
    }	   
  };    
  typedef _iterator	 iterator;
protected:
  SizeType        m_size;
  SizeType        m_offset;
public:
  var_vector()
  {
    m_size    = 0;
    m_offset  = 0;
  }
  var_vector(const SelfType&) = delete; 
  SelfType& operator=(const SelfType&) = delete;
public:
  SizeType          _total_bytes()const
  {
    return sizeof(SelfType) + _data_bytes();
  }
  SizeType          _data_bytes()const
  {
    return (m_size * sizeof(ValueType));
  }
  void              _set_offset(SizeType offset)
  {
    m_offset = offset;
  }
  ElementType*       _get_element(SizeType index)
  {
    ElementType* element = _get_data_addr();
    for(SizeType i = 0;i<index;i++)
      element = (ElementType*)( element->_get_data_addr() + element->_data_bytes() );
    return element;
  }
  const ElementType*       _get_element(SizeType index)const
  {
    const ElementType* element = _get_data_addr();
    for(SizeType i = 0;i<index;i++)
      element = (ElementType*)( element->_get_data_addr() + element->_data_bytes() );
    return element;
  }
public:
  void              prepare_append_elements(segment_manager& segment)
  {
    m_size = 0;
    _set_offset((SizeType)(segment.current() - (char*)this));
  }
public:
  ElementType*      begin_append_element(segment_manager& segment)
  {
    ElementType* new_element = (ElementType*)segment.current();
    if(!segment.advance( sizeof(ElementType) + sizeof(ValueType) ))
    {
      size_t free_size = segment.get_free_memory();
      size_t used_size = segment.size();
      size_t new_size = sizeof(ElementType) + sizeof(ValueType);
      std::string strErrMsg = "mmo_exception:: no enough memory,free:" + std::to_string(free_size) + ",used:"
        + std::to_string(used_size) + ",alloc size:" + std::to_string(new_size) ;
      throw mmo_exception((int32_t)mmo_exception::no_enough_memory,strErrMsg);
      return NULL;
    }

    //construct 
    ::new((void*)new_element)ElementType();
    //construct 
    ::new((void*)new_element->data())ValueType();
    return new_element;
  }
  bool              end_append_element(ElementType* element,segment_manager& segment)
  {
    size_t size = (segment.current() - ((char*)element + sizeof(ElementType)) );
    element->_set_data_size(size);
    m_size ++;
    
    return true;
  }
public:
  void              assign(const std::vector<ValueType>& src,segment_manager& segment)
  {
    prepare_append_elements(segment);
    for(auto& it:src)
    {
      auto element = begin_append_element(segment);
      element->object() = it;
      end_append_element(element,segment);
    }
  }
  void              assign(const std::list<ValueType>& src,segment_manager& segment)
  {
    prepare_append_elements(segment);
    for(auto& it:src)
    {
      auto element = begin_append_element(segment);
      element->object() = it;
      end_append_element(element,segment);
    }
  }
public:
  SizeType          size()const{ return m_size;}
  bool              empty()const{return m_size==0;}
public:
	iterator					begin()const 
  {
    if(empty()) 
      return end();
    return iterator((ElementType*)_get_data_addr(),0,m_size);
  }
	iterator					end()const	{return iterator(NULL,m_size,m_size);}
public:
  //在这个实现中，下标访问是低效的，不建议使用，推荐使用迭代器遍历
  ValueType&        operator[](SizeType index) {return _get_element(index)->object();}
  const ValueType&  operator[](SizeType index)const {return _get_element(index)->object();}
  ValueType&        at(SizeType index) {return _get_element(index)->object();}
  const ValueType&  at(SizeType index)const{return _get_element(index)->object();}
public:
  ValueType&          front(){return _get_element(0)->object();}
  const ValueType&    front()const {return _get_element(0)->object();}
  ValueType&          back(){return _get_element(m_size-1)->object();}
  const ValueType&    back()const {return _get_element(m_size-1)->object();}
protected:
  ElementType*        _get_data_addr()
  {
    return (ElementType*)((char*)this + m_offset);
  }
  const ElementType*  _get_data_addr()const
  {
    return (ElementType*)((char*)this + m_offset);
  }
};

template<typename KeyType,typename ValueType,typename SizeType>
class hash_node
{
  typedef hash_node<KeyType,ValueType,SizeType> SelfType;
public:
  offset_ptr<SelfType,SizeType>   next;
  KeyType                         key;
  ValueType                       value;
public:
  hash_node()
  {
    next = NULL;
  }
};

template<typename KeyType,typename ValueType,typename SizeType>
class hash_map
{
public:
  typedef hash_map<KeyType,ValueType,SizeType>  SelfType;
  typedef hash_node<KeyType,ValueType,SizeType> NodeType;
  typedef offset_ptr<NodeType,SizeType>         NodePtr;


  class iresult
  {
  public:
    bool           result{false};
    NodeType*      pvalue{NULL};
  public:
    iresult(){}
    iresult(bool ret,NodeType* pval):
    result(ret),
    pvalue(pval)
    {
    }
  };
  class iterator
  {
  public:
    SizeType                      index{0};
    offset_ptr<NodeType,size_t>   node;
    SelfType*                     self{nullptr};
  public:
    iterator(SelfType* pSelf,SizeType pos,NodeType* ptr)
    {
      index   =pos;
      node    =ptr;
      self    =pSelf; 
    }
    iterator(const iterator& other)
    {
      index   =other.index;
      node    =other.node;
      self    =other.self; 
    }
    iterator& operator=(const iterator& _rhs)
    {
      index   =_rhs.index;
      node    =_rhs.node;
      self    =_rhs.self; 
      return *this;
    }
    bool operator==(const iterator& _rhs) const
    {
      return (index == _rhs.index && 
              node == _rhs.node&& 
              self == _rhs.self );
    }
    bool operator!=(const iterator& _rhs) const
    {
      return (index != _rhs.index || 
              node != _rhs.node|| 
              self != _rhs.self );
    }
    ValueType* operator->() const
    {
      return (ValueType*)&node.get()->value;
    }
    ValueType& operator*() const
    {
      return (ValueType&)node.get()->value;
    }
    iterator operator++(int)
    {
      iterator _Tmp = *this;
      ++*this;
      return (_Tmp);
    }
    iterator& operator++()
    {	
      if(node != NULL)
      {
        node = node->next.get();
        if(node != NULL)
          return *this;
      }
      while(node == NULL && index < self->hash_size())
      {
        node = self->seek(++index);
      }
      return *this;
    }	   
  };
protected:
  SizeType    m_size{0};
  SizeType    m_key_table_size{0};
  SizeType    m_capacity{0};
  offset_ptr<NodePtr,SizeType> m_key_table;
  ValueType   m_default_value;
public:
  hash_map()
  {
    m_key_table = NULL;
  }
  hash_map(const SelfType& other)
  {
    m_size            = other.m_size;
    m_key_table_size  = other.m_key_table_size;
    m_capacity        = other.m_capacity;
    m_key_table       = other.m_key_table;
  } 
  SelfType& operator=(const SelfType& other)
  {
    m_size            = other.m_size;
    m_key_table_size  = other.m_key_table_size;
    m_capacity        = other.m_capacity;
    m_key_table       = other.m_key_table;
    return *this;
  }
public:
  static SizeType  predict_capacity_bytes(SizeType capacity,SizeType hash_size=0)
  {
    if(hash_size <= 0)
      hash_size = capacity;
    return sizeof(SelfType) + sizeof(NodePtr)*hash_size + sizeof(NodeType)*capacity ;
  }
  bool  init_hash(SizeType capacity,segment_manager& segment,SizeType hash_size=0)
  {
    if(m_key_table_size != 0)
      return false;
    m_capacity        = capacity;    
    m_key_table_size  = (hash_size <= 0)?capacity:hash_size;

    NodePtr* pNodes = (NodePtr*)segment.alloc(m_key_table_size*sizeof(NodePtr));
    if(pNodes == NULL)
    {
      m_capacity        = 0;
      m_key_table_size  = 0;
      throw mmo_exception((int32_t)mmo_exception::no_enough_memory,"mmo_exception:: no enough memory!");
      return false;
    }
    m_key_table         = pNodes;
    for(SizeType i=0;i<capacity;i++)
      pNodes[i]=NULL;
    return true;
  }
  SizeType capacity()const
  {
    return m_capacity;
  }
  SizeType hash_size()const
  {
    return m_key_table_size;
  }
  bool     add(KeyType key,const ValueType& value,segment_manager& segment)
  {
    if(m_size >= m_capacity)
      return false;//no enough space

    SizeType    index = key2index(key);
    NodeType*   n     = seek(index);
    if(n == NULL)
    {
      NodeType* v = (NodeType*)segment.alloc(sizeof(NodeType));
      if(v == NULL)
      {
        throw mmo_exception((int32_t)mmo_exception::no_enough_memory,"mmo_exception:: no enough memory!");
        return false;
      }  

      v->value = value;
      v->key   = key;
      v->next  = NULL;

      ++ m_size;
      m_key_table.get()[index] = v;
      return true;
    }

    while(n != NULL && n->key != key)
    {
      if(n->next == NULL)
      {
        NodeType* v = (NodeType*)segment.alloc(sizeof(NodeType));
        if(v == NULL)          
        {
          throw mmo_exception((int32_t)mmo_exception::no_enough_memory,"mmo_exception:: no enough memory!");
          return false;
        }  

        v->value = value;
        v->key   = key;
        v->next  = NULL;

        n->next = v;
        ++ m_size;
        return true;
      }        
      n = n->next.get();
    }
    return false;    
  }
  iresult  insert(KeyType key,const ValueType& value,segment_manager& segment)
  {
    if(m_size >= m_capacity)
      return iresult(false,NULL);//no enough space

    SizeType index = key2index(key);
    NodeType*  n = seek(index);
    if(n == NULL)
    {
      NodeType* v = (NodeType*)construct<NodeType>(segment);
      if(v == NULL)
        return iresult(false,NULL);

      v->value = value;
      v->key   = key;
      v->next  = NULL;

      ++ m_size;
      m_key_table.get()[index] = v;
      return iresult(true,v);
    }

    while(n != NULL && n->key != key)
    {
      if(n->next == NULL)
      {
        NodeType* v = (NodeType*)construct<NodeType>(segment);
        if(v == NULL)          
          return iresult(false,NULL);

        v->value = value;
        v->key   = key;
        v->next  = NULL;

        n->next = v;
        ++ m_size;
        return iresult(true,v);
      }        
      n = n->next.get();
    }
    return iresult(false,n);
  }
  ValueType&        operator[](const KeyType& key) 
  {
    NodeType*  n = seek(key2index(key));
    while(n != NULL&& n->key != key)
      n = n->next.get();
    return (n==NULL)?SelfType::m_default_value:n->value;
  }
  const ValueType&  operator[](const KeyType& key)const 
  {
    NodeType*  n = seek(key2index(key));
    while(n != NULL&& n->key != key)
      n = n->next.get();
    return (n==NULL)?SelfType::m_default_value:n->value;
  }
  iterator find(const KeyType& key)const 
  {
    SizeType index = key2index(key);
    NodeType*  n = seek(index);
    while(n!=NULL&& n->key != key)
      n = n->next.get();
    return (n == NULL)?end():iterator((SelfType*)this,index,n);
  }
  const ValueType* get(const KeyType& key)const
  {
    NodeType*  n = seek(key2index(key));
    while(n != NULL&& n->key != key)
      n = n->next.get();
    return (n==NULL)?NULL:&n->value;
  }
  ValueType* get(const KeyType& key)
  {
    NodeType*  n = seek(key2index(key));
    while(n != NULL&& n->key != key)
      n = n->next.get();
    return (n==NULL)?NULL:&n->value;
  }
  bool      empty()const
  {
    return m_size==0;
  }
  SizeType  size()const
  {
    return m_size;
  }
  iterator  begin()const
  {
    if(empty())
      return end();
    
    SizeType  index = 0;
    NodeType* node  = seek(index);
    while(node == NULL && index < m_key_table_size)
    {
      node = seek(++index);
    }
    return iterator((SelfType*)this,index,node);
  }
  iterator  end()const
  {
    return iterator((SelfType*)this,m_key_table_size,(NodeType*)NULL);
  }
protected:
  NodeType*     seek(SizeType index)const
  {
    NodePtr* pNodes = (NodePtr*)m_key_table.get();
    if(index < m_key_table_size)
      return pNodes[index].get();
    return (NodeType*)NULL;
  }
  SizeType  key2index(const KeyType& key)const
  {
    if(m_key_table_size != 0)
    {
      std::hash<KeyType>  key_hash;
      return SizeType( key_hash(key) )%m_key_table_size;
    } 
    return 1;
  }


};


#pragma pack(pop)





}//end namespace mmo