#ifndef GUIDPREFIX_h
#define GUIDPREFIX_h


namespace w2rp{

class guidPrefix
{
    public:

    void create()
    {

    }

    /**
     * Get a reference to the singleton instance.
     *
     * @return reference to the singleton instance.
     */
    static const guidPrefix& instance()
    {
        static guidPrefix singleton;
        return singleton;
    }
    
    private:

    guidPrefix()
    {

    }
    

};


} // end namespace w2rp

#endif