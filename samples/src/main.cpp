#include <gplay-engine.h>
using namespace gplay;


//! Game settings

// Pipe scrolling speed
#define PIPE_SCROLL_SPEED 100

// Timer in seconds for spawn a new pipe
#define PIPE_SPAWN_DELAY 1.6f

// Vertical force that affect bird
#define GRAVITY -700.0f

// Vertical force when bird is jumping
#define JUMP_VELOCITY 245.0f

// Ground vertical position, Bird will die if it reach this limit
#define GROUND_POS -182.0f

// Pipe settings
#define PIPE_SCROLL_X_START 200.0f
#define PIPE_SCROLL_X_END -200.0f
#define PIPE_HOLE_HEIGHT 42.0f
#define PIPE_HOLE_RANDOM_Y_OFFSET 90.0f

// File path for atlas sprite sheet
// used http://www.spritecow.com/ to get sprite coords from atlas
#define ATLAS_FILE "res/data/img/flappy_atlas.png"



/**
 * Interface for each entity in game.
 * Has a type and hold a node.
 */
class GameEntity : public Ref
{
public:

    enum E_ENTITY_TYPE
    {
        TYPE_NONE,
        TYPE_BIRD,
        TYPE_PIPE
    };

    GameEntity(E_ENTITY_TYPE type)
    {
        _type = type;
    }

    Node* getNode()
    {
        return _node;
    }

    E_ENTITY_TYPE type()
    {
        return _type;
    }

protected:
    Node* _node;
    E_ENTITY_TYPE _type;
};




/**
 * The Bird entity.
 */
class Bird : public GameEntity
{
    Animation* _birdAnimation;
    float _velocity;
    float _position;
    bool _isAlive;

public:

    Bird() : GameEntity(GameEntity::TYPE_BIRD)
      , _velocity(0.0f)
      , _position(0.0f)
      , _isAlive(true)
    {
        create();
    }

    ~Bird()
    {
        SAFE_RELEASE(_birdAnimation);
    }

    void restart()
    {
        _isAlive = true;
        _birdAnimation->play("fly");
        _node->setTranslation(Vector3(-50,0,0));

        _velocity = 0.0f;
        _position = 0.0f;
    }

    void jump()
    {
        if(_isAlive)
            _velocity = JUMP_VELOCITY;
    }

    void update(float dt)
    {
        _velocity += GRAVITY * dt;
        _position += _velocity * dt;

        if(_position < GROUND_POS)
            _position = GROUND_POS;

        _node->setTranslationY(_position);

    }

    void kill()
    {
        // final jump when dying (only for the graphic effect)
        jump();

        _isAlive = false;
        _birdAnimation->stop("fly");
    }

private:

    void create()
    {
        // Create animated bird sprite from the atlas image
        Sprite* spriteBird = Sprite::create(ATLAS_FILE, 34, 24, Rectangle(0,0,1024,1024), 3);
        spriteBird->setFrameSource(0, Rectangle(6,982,34,24));
        spriteBird->setFrameSource(1, Rectangle(62,982,34,24));
        spriteBird->setFrameSource(2, Rectangle(118,982,34,24));
        spriteBird->setOffset(Sprite::OFFSET_VCENTER_HCENTER);

        // Create a flying animation clip
        unsigned int keyTimes[3] = { 0, 1, 2 };
        float keyValues[3] =  { 0, 1, 2 };
        _birdAnimation = spriteBird->createAnimation("player-animations", Sprite::ANIMATE_KEYFRAME, 3, keyTimes, keyValues, Curve::LINEAR);
        _birdAnimation->createClip("fly", 0, 2)->setRepeatCount(AnimationClip::REPEAT_INDEFINITE);
        // Set the speed to 24 FPS
        _birdAnimation->getClip("fly")->setSpeed(12.0f / 1000.0f);
        _birdAnimation->play("fly");

        // Create node
        _node = Node::create("bird");
        _node->setDrawable(spriteBird);
        _node->setTranslation(Vector3(-50,0,0));

        // Add collision sphere
        _node->setCollisionObject(PhysicsCollisionObject::GHOST_OBJECT, PhysicsCollisionShape::sphere(13));

        SAFE_RELEASE(spriteBird);
    }
};



/**
 * The Pipe entity
 *
 */
class Pipe : public GameEntity
{
    PhysicsCollisionObject* _collider1;
    PhysicsCollisionObject* _collider2;

public:

    Pipe() : GameEntity(GameEntity::TYPE_PIPE)
    {
        create();
    }

    ~Pipe()
    {
        _node->removeAllChildren();
        SAFE_RELEASE(_node);
        SAFE_DELETE(_collider1);
        SAFE_DELETE(_collider2);
    }

    void update(float dt)
    {
        _node->translateX(-PIPE_SCROLL_SPEED * dt);
    }

    bool isOffScreen()
    {
        return _node->getTranslationX() < PIPE_SCROLL_X_END;
    }

private:

    void create()
    {
        const float pipeHeight = 320.0f;

        float UP_OFFSET = pipeHeight / 2 + PIPE_HOLE_HEIGHT;
        float DOWN_OFFSET = -pipeHeight / 2 - PIPE_HOLE_HEIGHT;

        // up pipe child
        Sprite* pipeUp = Sprite::create(ATLAS_FILE, 52, 320, Rectangle(112,646,52,320));
        pipeUp->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* nodePipeUp = Node::create("pipeUp");
        nodePipeUp->setTranslation(Vector3(0,UP_OFFSET,0));
        nodePipeUp->setDrawable(pipeUp);
        _collider1 = nodePipeUp->setCollisionObject(PhysicsCollisionObject::GHOST_OBJECT, PhysicsCollisionShape::box(Vector3(52,320,0)));
        SAFE_RELEASE(pipeUp);

        // down pipe child
        Sprite* pipeDown = Sprite::create(ATLAS_FILE, 52, 320, Rectangle(168,646,52,320));
        pipeDown->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* nodePipeDown = Node::create("pipeDown");
        nodePipeDown->setTranslation(Vector3(0,DOWN_OFFSET,0));
        nodePipeDown->setDrawable(pipeDown);
        _collider2 = nodePipeDown->setCollisionObject(PhysicsCollisionObject::GHOST_OBJECT, PhysicsCollisionShape::box(Vector3(52,320,0)));
        SAFE_RELEASE(pipeDown);

        // merge up and down pipes to the parent node
        _node = Node::create("pipe");
        _node->addChild(nodePipeUp);
        _node->addChild(nodePipeDown);

        // add this entity into the up and down pipes nodes
        // it will be used in collisions detection to get object type that collides
        nodePipeUp->setUserObject(this);
        nodePipeDown->setUserObject(this);

        // set random initial vertical position
        _node->setTranslation(PIPE_SCROLL_X_START, MATH_RANDOM_MINUS1_1() * PIPE_HOLE_RANDOM_Y_OFFSET, 0);
    }
};



/**
 * The GplayFlappyBird game class
 */
class GplayFlappyBird : public Game, public PhysicsCollisionObject::CollisionListener
{
    Scene* _scene;
    Bird* _bird;
    std::list<Pipe*> _pipes;
    bool _tapping;
    float _timer;
    bool _showPhysicsDebug;

    Node* _nodeMenu;
    Node* _nodeGameOver;


    enum GameState
    {
        GameMenu = 1,
        GameRunning = 2,
        GameOver = 3
    };

    GameState _currentState;


public:

    GplayFlappyBird() :
        _scene(nullptr)
    {
        _tapping = false;
        _showPhysicsDebug = false;
        _currentState = GameMenu;
    }

    void finalize()
    {
        removeAllPipes();
        SAFE_DELETE(_bird);
        SAFE_RELEASE(_scene);
    }


    Node* _nodeLayerBack;
    Node* _nodeLayerPipes;
    Node* _nodeLayerHero;
    Node* _nodeLayerMenu;


    void initialize() override
    {
        // create the scene.
        _scene = Scene::create();

        // create an ortho camera
        Camera* camera = Camera::createOrthographic(getWidth(), getHeight(), getAspectRatio(), -100.0, 100.0);
        Node* cameraNode = _scene->addNode("camera");
        cameraNode->setCamera(camera);
        _scene->setActiveCamera(camera);

        // create 4 root nodes to act as layers for ordering draws by group (z ordering)
        // next game nodes will be added as childs in those layers.
        _nodeLayerBack = Node::create("layer_back");
        _nodeLayerPipes = Node::create("layer_pipes");
        _nodeLayerHero = Node::create("layer_hero");
        _nodeLayerMenu = Node::create("layer_menu");
        _scene->addNode(_nodeLayerBack);
        _scene->addNode(_nodeLayerPipes);
        _scene->addNode(_nodeLayerHero);
        _scene->addNode(_nodeLayerMenu);

        // create background
        Sprite* spriteBackground = Sprite::create(ATLAS_FILE, 288, 512, Rectangle(0,0,288,512));
        spriteBackground->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* nodeBackground = Node::create("background");
        nodeBackground->setTranslation(Vector3(0,0,0));
        nodeBackground->setDrawable(spriteBackground);
        _nodeLayerBack->addChild(nodeBackground);

        // create ground
        Sprite* spriteGround = Sprite::create(ATLAS_FILE, 336, 112, Rectangle(584,0,336,112));
        spriteGround->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* nodeGround = Node::create("ground");
        nodeGround->setTranslation(Vector3(0,-250,0));
        nodeGround->setDrawable(spriteGround);
        _nodeLayerHero->addChild(nodeGround);

        // create bird
        _bird = new Bird();
        _bird->getNode()->getCollisionObject()->addCollisionListener(this);
        _nodeLayerHero->addChild(_bird->getNode());

        // create start menu sprites
        {
        Sprite* startMenu = Sprite::create(ATLAS_FILE, 196, 62, Rectangle(584,116,196,62));
        startMenu->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* nodeStart = Node::create("start");
        nodeStart->setTranslation(Vector3(0,100,0));
        nodeStart->setDrawable(startMenu);

        Sprite* tap = Sprite::create(ATLAS_FILE, 114, 98, Rectangle(584,182,114,98));
        tap->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* nodeTap = Node::create("instructions");
        nodeTap->setTranslation(Vector3(0,0,0));
        nodeTap->setDrawable(tap);

        _nodeMenu = Node::create("menu");
        _nodeMenu->addChild(nodeStart);
        _nodeMenu->addChild(nodeTap);
        _nodeMenu->setEnabled(false);
        _nodeLayerMenu->addChild(_nodeMenu);
        }

        // create game over sprites
        {
        Sprite* gameOverSprite = Sprite::create(ATLAS_FILE, 204, 54, Rectangle(784,116,204,54));
        gameOverSprite->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        _nodeGameOver = Node::create("gameover");
        _nodeGameOver->setTranslation(Vector3(0,100,0));
        _nodeGameOver->setDrawable(gameOverSprite);
        _nodeGameOver->setEnabled(false);
        _nodeLayerMenu->addChild(_nodeGameOver);

        Sprite* playSprite = Sprite::create(ATLAS_FILE, 116, 70, Rectangle(702,234,116,70));
        playSprite->setOffset(Sprite::OFFSET_VCENTER_HCENTER);
        Node* node = Node::create("play");
        node->setDrawable(playSprite);
        node->setTranslation(Vector3(0,-100,0));
        _nodeGameOver->addChild(node);
        }
    }

    void gameOver()
    {
        _bird->kill();
        _currentState = GameOver;
    }

    void newGame()
    {
        removeAllPipes();
        _nodeMenu->setEnabled(false);
        _nodeGameOver->setEnabled(false);
        _timer = 0.0f;
        _bird->restart();
        _currentState = GameRunning;

        // Create the first pipe immediatly (without the timer)
        addPipe();
    }

    void collisionEvent(PhysicsCollisionObject::CollisionListener::EventType type,
                        const PhysicsCollisionObject::CollisionPair& collisionPair,
                        const Vector3& contactPointA,
                        const Vector3& contactPointB) override
    {
        // objectA -> bird
        // objectB -> any collider
        // However we only care about collisions between the bird and a pipe

        if (type == PhysicsCollisionObject::CollisionListener::COLLIDING && collisionPair.objectB)
        {
            GameEntity* entity = (GameEntity*)collisionPair.objectB->getNode()->getUserObject();
            if(entity && entity->type() == GameEntity::TYPE_PIPE)
            {
                gameOver();
            }
        }
    }

    void addPipe()
    {
        Pipe* pipe = new Pipe();
        _pipes.push_back(pipe);
        _nodeLayerPipes->addChild(pipe->getNode());
    }

    void removeAllPipes()
    {
        // delete all pipes
        for(auto&& pipe : _pipes)
        {
            pipe->release();
        }

        // clear pipe list
        _pipes.clear();
    }

    void update(float elapsedTime) override
    {
        // get elpased time in seconds
        float deltaTime = (float)elapsedTime / 1000.0;

        switch(_currentState)
        {
        case GameMenu:
            runGameMenu();
            break;
        case GameOver:
            runGameOver(deltaTime);
            break;
        case GameRunning:
            runGameLevel(deltaTime);
            break;
        }
    }

    void runGameLevel(float elapsedTime)
    {
        // update bird position, if game is over, bird will fall down
        _bird->update(elapsedTime);

        // generate a new pipe at each timer delay
        _timer += elapsedTime;
        if(_timer > PIPE_SPAWN_DELAY)
        {
            addPipe();
            _timer = 0.0f;
        }

        // loop through each pipes
        auto it = _pipes.begin();
        while(it != _pipes.end())
        {
            Pipe* pipe = (*it);

            // update pipe position
            pipe->update(elapsedTime);

            // remove pipe when offscreen
            if(pipe->isOffScreen())
            {
                _scene->removeNode(pipe->getNode());
                delete pipe;
                it = _pipes.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // jump ?
        if(_tapping)
        {
            _bird->jump();
            _tapping = false;
        }

        // check ground limits
        if(_bird->getNode()->getTranslationY() <= GROUND_POS)
            gameOver();
    }

    void runGameOver(float elapsedTime)
    {
        // update bird position, when game is over, bird will fall down until limits
        _bird->update(elapsedTime);
        _nodeGameOver->setEnabled(true);
        _nodeGameOver->getFirstChild()->setEnabled(false);

        // wait 2 second until it's possible to restart the game
        _timer += elapsedTime;
        if(_timer < 2.0f)
        {
            return;
        }

        _nodeGameOver->getFirstChild()->setEnabled(true);
        if(_tapping)
        {
            newGame();
        }

    }

    void runGameMenu()
    {
        _nodeMenu->setEnabled(true);

        if(_tapping)
        {
            newGame();
        }
    }

    void render(float elapsedTime) override
    {
        clear(ClearFlags::CLEAR_COLOR_DEPTH, Vector4::fromColor(0x45678ff), 1.0f, 0);

        // visit scene
        _scene->visit(this, &GplayFlappyBird::drawScene);

        // draw physics debug
        if(_showPhysicsDebug)
            Game::getInstance()->getPhysicsController()->drawDebug(_scene->getActiveCamera()->getViewProjectionMatrix());
    }

    bool drawScene(Node* node)
    {
        if(node->isEnabled() == false)
            return false;

        Drawable* drawable = node->getDrawable();
        if (drawable)
            drawable->draw();

        return true;
    }

    void keyEvent(Keyboard::KeyEvent evt, int key) override
    {
        if (evt == Keyboard::KEY_PRESS)
        {
            switch (key)
            {
            case Keyboard::KEY_B:
                _showPhysicsDebug = !_showPhysicsDebug;
                break;
            }
        }
    }

    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex) override
    {
        switch(evt)
        {
        case Touch::TOUCH_PRESS:
            _tapping = true;
            break;

        case Touch::TOUCH_MOVE:
            break;

        case Touch::TOUCH_RELEASE:
            _tapping = false;
            break;
        }
    }
};

// Declare our game instance
GplayFlappyBird __flappyGame;
