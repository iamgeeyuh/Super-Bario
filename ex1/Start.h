#include "Scene.h"

class Start : public Scene {
public:
    int ENEMY_COUNT = 0;
    
    ~Start();
    
    void initialise() override;
    void update(float delta_time) override;
    void render(ShaderProgram *program) override;
};
