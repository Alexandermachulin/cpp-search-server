#pragma once

#include <vector>

#include <utility>

template<typename It>
struct IteratorRange {
         It begin_r;
         It end_r;  
};

template<typename It>
class Paginator {
    public:
        Paginator(It begin, It end, size_t page_size) 
        {
                for(It i = begin; i < end; advance(i, page_size)) {
                    if (distance(begin, end) <= page_size) {
                        paginated_.push_back({begin, end});
                        break;
                    }
                    else if(distance(i, end) == 1) {
                        paginated_.push_back({i, i + 1});
                        
                    }
                    else {
                        paginated_.push_back({i, i + page_size });
                    }
                }
        }
        
        auto begin() const {
            return paginated_.begin();
        }
        
        auto end() const {
            return paginated_.end();
        }
        
        auto size() {
            return paginated_.size();
        }
    
    private:
        std::vector<IteratorRange<It>> paginated_;
};

template <typename Iterator>
std::ostream &operator<<(std::ostream &out, const IteratorRange<Iterator> &It)
{
    for (auto i = It.begin_r; i != It.end_r; ++i)
        out << *i;
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
