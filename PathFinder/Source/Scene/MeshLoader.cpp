#include "MeshLoader.hpp"

namespace PathFinder
{

    MeshLoader::MeshLoader(const std::filesystem::path& fileRoot, VertexStorage* gpuVertexStorage)
        : mRootPath{ fileRoot }, mVertexStorage{ gpuVertexStorage } {}

    std::vector<Mesh> MeshLoader::Load(const std::string& fileName)
    {
        Assimp::Importer importer;
        std::string fullPath{ mRootPath.append(fileName).string() };

        auto postProcessSteps = (aiPostProcessSteps)(
            aiProcess_CalcTangentSpace | 
            //aiProcess_FlipUVs | // Important for DirectX. Incorrect UV will also affect tangent/bitangent calculation.
            aiProcess_GenUVCoords | 
            aiProcess_GenNormals | 
            aiProcess_JoinIdenticalVertices);

        const aiScene* pScene = importer.ReadFile(fullPath, postProcessSteps);

        if (!pScene) throw std::invalid_argument("Unable to read mesh file");

        mLoadedMeshes.clear();

        ProcessNode(pScene->mRootNode, pScene);

        return mLoadedMeshes;
    }

    Mesh MeshLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        Mesh subMesh;

        // Walk through each of the mesh's vertices
        for (auto i = 0u; i < mesh->mNumVertices; i++)
        {
            Vertex1P1N1UV1T1BT vertex{};

            vertex.Position.x = mesh->mVertices[i].x;
            vertex.Position.y = mesh->mVertices[i].y;
            vertex.Position.z = mesh->mVertices[i].z;
            vertex.Position.w = 1.0; 

            if (mesh->HasTextureCoords(0))
            {
                vertex.UV.x = (float)mesh->mTextureCoords[0][i].x;
                vertex.UV.y = (float)mesh->mTextureCoords[0][i].y;
            }

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.Tangent.x = mesh->mTangents[i].x;
                vertex.Tangent.y = mesh->mTangents[i].y;
                vertex.Tangent.z = mesh->mTangents[i].z;

                vertex.Bitangent.x = mesh->mBitangents[i].x;
                vertex.Bitangent.y = mesh->mBitangents[i].y;
                vertex.Bitangent.z = mesh->mBitangents[i].z;
            }

            if (mesh->HasNormals())
            {
                vertex.Normal.x = mesh->mNormals[i].x;
                vertex.Normal.y = mesh->mNormals[i].y;
                vertex.Normal.z = mesh->mNormals[i].z;
            }

            subMesh.AddVertex(vertex);
        }

        for (auto i = 0u; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            for (auto j = 0u; j < face.mNumIndices; j++) 
            {
                subMesh.AddIndex(face.mIndices[j]);
            }
        }

        subMesh.SetName(mesh->mName.data);

        VertexStorageLocation vsLocation = mVertexStorage->AddVertices(
            subMesh.Vertices().data(), subMesh.Vertices().size(), subMesh.Indices().data(), subMesh.Indices().size()
        );

        subMesh.SetVertexStorageLocation(vsLocation);

        return subMesh;
    }

    void MeshLoader::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (auto i = 0u; i < node->mNumMeshes; i++)
        {
            aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
            mLoadedMeshes.emplace_back(ProcessMesh(assimpMesh, scene));
        }

        for (auto i = 0u; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }


    void MeshLoader::CalculateTangentSpace(Mesh* mesh)
    {
    //    // Walk through each of the mesh's vertices
    //    for (auto i = 0u; i < mesh->Vertices().size(); i += 3)
    //    {
    //        
    //        Vertex1P1N1UV1T1BT& vertex0 = mesh->Vertices()[i];
    //        Vertex1P1N1UV1T1BT& vertex1 = mesh->Vertices()[i + 1];
    //        Vertex1P1N1UV1T1BT& vertex2 = mesh->Vertices()[i + 2];


    //        // Given the 3 vertices (position and texture coordinates) of a triangle
    //// calculate and return the triangle's tangent vector.

    //// Create 2 vectors in object space.
    ////
    //// edge1 is the vector from vertex positions pos1 to pos2.
    //// edge2 is the vector from vertex positions pos1 to pos3.
    //        glm::vec3 edge1(pos2[0] - pos1[0], pos2[1] - pos1[1], pos2[2] - pos1[2]);
    //        glm::vec3 edge2(pos3[0] - pos1[0], pos3[1] - pos1[1], pos3[2] - pos1[2]);

    //        edge1.normalize();
    //        edge2.normalize();

    //        // Create 2 vectors in tangent (texture) space that point in the same
    //        // direction as edge1 and edge2 (in object space).
    //        //
    //        // texEdge1 is the vector from texture coordinates texCoord1 to texCoord2.
    //        // texEdge2 is the vector from texture coordinates texCoord1 to texCoord3.
    //        glm::vec2 texEdge1(texCoord2[0] - texCoord1[0], texCoord2[1] - texCoord1[1]);
    //        glm::vec2 texEdge2(texCoord3[0] - texCoord1[0], texCoord3[1] - texCoord1[1]);

    //        texEdge1.normalize();
    //        texEdge2.normalize();

    //        // These 2 sets of vectors form the following system of equations:
    //        //
    //        //  edge1 = (texEdge1.x * tangent) + (texEdge1.y * bitangent)
    //        //  edge2 = (texEdge2.x * tangent) + (texEdge2.y * bitangent)
    //        //
    //        // Using matrix notation this system looks like:
    //        //
    //        //  [ edge1 ]     [ texEdge1.x  texEdge1.y ]  [ tangent   ]
    //        //  [       ]  =  [                        ]  [           ]
    //        //  [ edge2 ]     [ texEdge2.x  texEdge2.y ]  [ bitangent ]
    //        //
    //        // The solution is:
    //        //
    //        //  [ tangent   ]        1     [ texEdge2.y  -texEdge1.y ]  [ edge1 ]
    //        //  [           ]  =  -------  [                         ]  [       ]
    //        //  [ bitangent ]      det A   [-texEdge2.x   texEdge1.x ]  [ edge2 ]
    //        //
    //        //  where:
    //        //        [ texEdge1.x  texEdge1.y ]
    //        //    A = [                        ]
    //        //        [ texEdge2.x  texEdge2.y ]
    //        //
    //        //    det A = (texEdge1.x * texEdge2.y) - (texEdge1.y * texEdge2.x)
    //        //
    //        // From this solution the tangent space basis vectors are:
    //        //
    //        //    tangent = (1 / det A) * ( texEdge2.y * edge1 - texEdge1.y * edge2)
    //        //  bitangent = (1 / det A) * (-texEdge2.x * edge1 + texEdge1.x * edge2)
    //        //     normal = cross(tangent, bitangent)

    //        glm::vec3 t;
    //        glm::vec3 b;
    //        glm::vec3 n(normal[0], normal[1], normal[2]);

    //        float det = (texEdge1.x * texEdge2.y) - (texEdge1.y * texEdge2.x);

    //        if (Math::closeEnough(det, 0.0f))
    //        {
    //            t.set(1.0f, 0.0f, 0.0f);
    //            b.set(0.0f, 1.0f, 0.0f);
    //        }
    //        else
    //        {
    //            det = 1.0f / det;

    //            t.x = (texEdge2.y * edge1.x - texEdge1.y * edge2.x) * det;
    //            t.y = (texEdge2.y * edge1.y - texEdge1.y * edge2.y) * det;
    //            t.z = (texEdge2.y * edge1.z - texEdge1.y * edge2.z) * det;

    //            b.x = (-texEdge2.x * edge1.x + texEdge1.x * edge2.x) * det;
    //            b.y = (-texEdge2.x * edge1.y + texEdge1.x * edge2.y) * det;
    //            b.z = (-texEdge2.x * edge1.z + texEdge1.x * edge2.z) * det;

    //            t = glm::normalize(t);
    //            b = glm::normalize(b);
    //        }

    //        // Calculate the handedness of the local tangent space.
    //        // The bitangent vector is the cross product between the triangle face
    //        // normal vector and the calculated tangent vector. The resulting bitangent
    //        // vector should be the same as the bitangent vector calculated from the
    //        // set of linear equations above. If they point in different directions
    //        // then we need to invert the cross product calculated bitangent vector. We
    //        // store this scalar multiplier in the tangent vector's 'w' component so
    //        // that the correct bitangent vector can be generated in the normal mapping
    //        // shader's vertex shader.

    //        glm::vec3 bitangent = glm::cross(n, t);
    //        float handedness = (glm::dot(bitangent, b) < 0.0f) ? -1.0f : 1.0f;

    //        tangent[0] = t.x;
    //        tangent[1] = t.y;
    //        tangent[2] = t.z;
    //        tangent[3] = handedness;
    }

}
