#pragma once
#include "Geography/Geo.h"
#include "Wx.h"


#include <filesystem>
#include <unordered_map>
#include <vector>

struct WxAtGeoCoord {
    GeoCoord coord;
    Wx wx;
};

struct QuadIndexes {
    int32_t topLeft, topRight, bottomLeft, bottomRight;
};

class GribData {
private:    
    const static WxAtGeoCoord emptyWx;

    const size_t numberOfFiles;
    const std::vector<int32_t> validIndexes;
    const std::vector<QuadIndexes> quadIndexes;
    const std::unordered_map<GeoCoord, int32_t> geoCoordLookup;
    const std::vector<std::unordered_map<int32_t, WxAtGeoCoord>> wxResults;

public:
    GribData(std::vector<int32_t>& validIndexes, const std::vector<QuadIndexes>& quadIndexes, std::unordered_map<GeoCoord, int32_t>& geoCoordLookup, std::vector<std::unordered_map<int32_t, WxAtGeoCoord>>& wxResults) 
        : numberOfFiles(wxResults.size()), validIndexes(validIndexes), quadIndexes(quadIndexes), geoCoordLookup(geoCoordLookup), wxResults(wxResults) {}

    inline size_t GetNumberOfFiles() { return numberOfFiles; }
    inline const WxAtGeoCoord& GetWxAtGeoCoord(const GeoCoord& geoCoord, int32_t fileIndex)
    {
        if(fileIndex < 0 || fileIndex >= numberOfFiles)
            return emptyWx;

        auto indexItr = geoCoordLookup.find(geoCoord);
        if(indexItr == geoCoordLookup.end())
            return emptyWx;

        return wxResults[fileIndex].find(indexItr->second)->second;
    }

    std::vector<GeoCoord> GetGeoCoords();
    const std::vector<QuadIndexes>& GetQuadIndexes() { return quadIndexes; }

    void Save(std::filesystem::path path);
    static GribData* Load(std::filesystem::path path);

    struct Quad {
        WxAtGeoCoord topLeft, topRight, bottomLeft, bottomRight;
    };

    class QuadIterator {
    private:
        const int32_t fileIndex;
        const std::unordered_map<int32_t, WxAtGeoCoord>& wxResult;
        std::vector<QuadIndexes>::const_iterator currentQuadItr;
        std::vector<QuadIndexes>::const_iterator quadItrEnd;
        Quad currentQuad;

        void UpdateCurrentQuad()
        {
            if(currentQuadItr != quadItrEnd)
            {
                auto& currentQuadIndexes = *currentQuadItr;
                currentQuad = {
                    .topLeft = wxResult.find(currentQuadIndexes.topLeft)->second,
                    .topRight = wxResult.find(currentQuadIndexes.topRight)->second,
                    .bottomLeft = wxResult.find(currentQuadIndexes.bottomLeft)->second,
                    .bottomRight = wxResult.find(currentQuadIndexes.bottomRight)->second
                };
            }
            else
                currentQuad = {emptyWx, emptyWx, emptyWx, emptyWx};
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Quad;
        using pointer = const value_type*;
        using reference = const value_type&;
        using difference_type = ptrdiff_t;

        inline QuadIterator(int32_t fileIndex, const std::unordered_map<int32_t, WxAtGeoCoord>& wxResult, std::vector<QuadIndexes>::const_iterator itr, std::vector<QuadIndexes>::const_iterator itrEnd)
            : fileIndex(fileIndex), wxResult(wxResult), currentQuadItr(itr), currentQuad({emptyWx, emptyWx, emptyWx, emptyWx}), quadItrEnd(itrEnd)
            {
                UpdateCurrentQuad();
            }

        inline reference operator*() const {
            return currentQuad;
        }

        inline pointer operator->() const {
            return &currentQuad;
        }

        inline friend bool operator==(const QuadIterator& lhs, const QuadIterator& rhs) {
            return lhs.fileIndex == rhs.fileIndex && lhs.currentQuadItr == rhs.currentQuadItr;
        }

        inline friend bool operator!=(const QuadIterator& lhs, const QuadIterator& rhs) {
            return !(lhs == rhs);
        }

        inline QuadIterator& operator++() {
            currentQuadItr++;
            UpdateCurrentQuad();
            return *this;
        }

        inline QuadIterator operator++(int) {
            auto result = *this;
            currentQuadItr++;
            UpdateCurrentQuad();
            return result;
        }
    };

    class GribDataFileQuads {
        private:
            int32_t fileIndex;
            GribData* parent;

        public:
            GribDataFileQuads(GribData* parent, int32_t fileIndex) : parent(parent), fileIndex(fileIndex) {}

        inline QuadIterator begin() const {
            return QuadIterator(fileIndex, parent->wxResults[fileIndex], parent->quadIndexes.begin(), parent->quadIndexes.end());
        }

        inline QuadIterator end() const {
            return QuadIterator(fileIndex, parent->wxResults[fileIndex], parent->quadIndexes.end(), parent->quadIndexes.end());
        }
    };

    inline GribDataFileQuads GetQuadIteratorForFileIndex(int32_t fileIndex) {
        return GribDataFileQuads(this, fileIndex);
    }
};