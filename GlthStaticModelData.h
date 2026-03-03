// GlthStaticModelData.h

#ifndef GLTH_STATIC_MODEL_DATA_H
#define GLTH_STATIC_MODEL_DATA_H

#include <string>
#include <vector>

class GlthStaticModelData {
public:
    GlthStaticModelData();
    ~GlthStaticModelData();

    void LoadModel(const std::string& modelPath);
    void Render();

    // New material support methods
    void SetMaterial(const std::string& materialName);
    std::string GetMaterial() const;

private:
    std::string m_materialName;
    // ... other data members for static model
};

#endif // GLTH_STATIC_MODEL_DATA_H
