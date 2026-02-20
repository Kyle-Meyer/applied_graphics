In Project 2 we will extend the simple scene graph representation from 605.667 Computer Graphics. The scene graph will be extended to support hierarchical bounding volumes. Scene graph traversal methods will be extended to support view frustum culling and level of detail selection.

You will define a scene of your choice. The primary focus is on optimizing the scene graph traversal to highlight optimization techniques. 
Focus is on using hierarchical bounding volumes within a more complex scene where culling and level of detail techniques can be effective. 
In addition, exploration of a procedural modeling technique such as particle systems or parametric technique is required. 
Two of the techniques listed below must be included in the scene:


- Parametric surfaces
    - You can subdivide and store in a mesh surface
- Parametric curves
    - An example use would be to define motion of an object (or the camera) using a parametric curve
    - Create an animated sequence that updates the position using a forward differencing solution of the parametric equation
- Subdivision surfaces
    - You can either render on the fly or pre-generate different levels of subdivision and store in a mesh surface.

You should be able to find some sample subdivision surface meshes or parametric surface control point representations on the web.

You must implement the following 2 techniques within your scene graph:
- View frustum culling
    - This involves extending the Camera class to extract view frustum planes and provide frustum/BV intersection testing
    - Demonstrate hierarchical view frustum culling within your scene graph for at least one object
        - i.e. one object must have children that also have BVs
- Multiple Levels of Detail with some selection criteria on at least 1 object in your scene
    - Parametric surfaces and subdivision surfaces are ideal to demonstrate this. 
    A sphere (mesh object) is also a good choice. You may also be able to find models on the web that have different detail levels. 
    Selection can be made based on distance or an estimate of projected area of a bounding sphere.

You may augment your scene with other polygon mesh objects where necessary to provide interest as well 
as to demonstrate view-frustum culling and perhaps multiple LODs.


Common Requirements
- Must exhibit view control
    - It is sufficient to use existing Camera view methods. If you extend the Camera motion methods, please document. I need to be able to move the camera around.
    - Also, place a print statement in your code that identifies which object(s) are culled due to view-frustum culling and which detail level is used for the level of detail requirement.
        - This will allow me to move around and see that objects are being removed or detail level being changed.
    - You may add controls to change any options via keyboard selections.
- Documentation
     - Project Description
        - This should describe your scene definition, how you code is structured, and should include brief descriptions of the techniques.
        - Submit a document or provide a text submission
- Screenshots 
    - an overview screen shot (required)
    - other screen shots that focus on particular objects (recommended)

Grading
This assignment is worth 30 points (30% of the total course grade) allocated as follows:

7 points - Technique 1
7 points - Technique 2
7 points - View frustum culling
5 points - Level of Detail selection
2 points - Scene Definition
2 points - Comments, Project Description, Screenshot(s)
