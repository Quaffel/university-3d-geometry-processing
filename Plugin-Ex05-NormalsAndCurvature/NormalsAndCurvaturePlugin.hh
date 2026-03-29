#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/LoadSaveInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <ObjectTypes/TriangleMesh/TriangleMeshTypes.hh>
//#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
//
#include <OpenMesh/Core/Utils/PropertyManager.hh>

class NormalsAndCurvatureToolbar;

enum class VertexNormalKind {
    UniformInterpol,
    AngleBasedInterpol,
    AreaBasedInterpol,
    MeanCurvature,
};

enum class LaplacianWeights {
    Uniform,
    Cotan,
};
enum class AreaFormulation {
    Barycentric,  // Each face area divided equally among vertices
    MixedVoronoi, // Mixed weights according to Meyer '03, Discrete Differential-Geometry Operators for Triangulated 2-Manifolds
};
enum class CurvatureKind {
    Gaussian,
    IntegratedGaussian,
    Mean,
    Min,
    Max,
};
using ScalarVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<double>>;
using ScalarEProp = OpenMesh::PropertyManager<OpenMesh::EPropHandleT<double>>;
using ScalarHProp = OpenMesh::PropertyManager<OpenMesh::HPropHandleT<double>>;
using VectorVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<ACG::Vec3d>>;

class NormalsAndCurvaturePlugin : public QObject, BaseInterface, LoadSaveInterface, ToolboxInterface, LoggingInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(LoadSaveInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex05-NormalsAndCurvaturePlugin")
public:
    QString name() override { return (QString("Ex05-NormalsAndCurvaturePlugin")); };
    QString description( ) override { return (QString("Geometry Processing Exercise 05")); };

signals:
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;

    // BaseInterface:
    void updatedObject(int _identifier, const UpdateType& _type) override;
    void addEmptyObject(DataType _type, int& _objectId) override;

public slots:
    // BaseInterface:
    void initializePlugin() override;

    // our own slots:
    void slotImportOVMSurfaces();
    void slotUpdateNormals(VertexNormalKind);
    void slotShowCurvature();

private:
    /// Compute per-vertex normals and store them in the standard mesh normal attribute.
    void computeVertexNormals(TriMesh&, VertexNormalKind);
    /// Compute interpolation weights to interpolate per-face values to vertices.
    /// One value per mesh half-edge is the weight of the halfedge's associated face
    /// when interpolating the value for the vertex that the halfedge points at.
    ScalarHProp computeNormalInterpolationWeights(TriMesh&, VertexNormalKind);
    /// We only use symmetric weights. For non-symmetric weights, you could store them
    /// as a halfedge property instead of an edge property.
    ScalarEProp computeCotanWeights(TriMesh&);
    ScalarEProp computeUniformLaplacianWeights(TriMesh&);

    /// Compute per-vertex areas for mass matrix
    ScalarVProp computeVertexAreas(TriMesh&);
    /// Compute pair(point-wise gaussian curvature, angle defect)
    std::pair< ScalarVProp, ScalarVProp> computeGaussianCurvature(TriMesh&);
    ScalarVProp computeMeanCurvature(TriMesh&, LaplacianWeights _lap_kind);
    /// Compute the mean curvature normal
    VectorVProp computeMeanCurvatureNormal(TriMesh&, LaplacianWeights _lap_kind);
    /// Compute pair(min, max) based on gaussian and mean curvature
    std::pair<ScalarVProp, ScalarVProp> computeMinMaxCurvature(TriMesh&, LaplacianWeights _lap_kind);

private:
    NormalsAndCurvatureToolbar *tool_;
};
