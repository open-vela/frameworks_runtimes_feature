template <typename T>
class FTArrayHelper {
private:
    FtArray* _data;

public:
    FTArrayHelper(FtArray* data)
    {
        _data = data;
    }

    ~FTArrayHelper()
    {
    }

    T& operator[](int32_t index)
    {
        return ((T*)_data->_element)[index];
    }

    int32_t size() const { return _data->_size; }
};