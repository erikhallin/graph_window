#include <windows.h>
#include <gl/gl.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

struct st_point
{
    st_point()
    {
        x=0;
        y=0;
    }
    st_point(float _x,float _y)
    {
        x=_x;
        y=_y;
    }

    float x,y;
};

//window size is the whole window
float g_window_width=600;
float g_window_height=300;
//screen is the area where the graph is shown
float g_screen_width=550;
float g_screen_height=230;
//input
float g_mouse_pos[2]={0,0};
bool  g_mouse_but[4]={false,false,false,false};
bool  g_key_trigger_left=false;
bool  g_key_trigger_right=false;
//variables
float g_window_border_shift=15;
float g_scale1_x=1.0;
float g_scale1_y=1.0;
float g_scale2_x=1.0;
float g_scale2_y=1.0;
float g_zoom1_y=0.6;
float g_zoom2_y=0.9;
bool  g_zoom_target_2=false;
float g_value_to_draw_x=0;
float g_value_to_draw_y=0;
float g_text_scale=6;
float g_text_gap_size=0.4;
int   g_frame_shift_counter=0;

vector<st_point> g_vec_points_s1;
vector<st_point> g_vec_points_s2;
vector<st_point> g_vec_points_org_s1;
vector<st_point> g_vec_points_org_s2;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND, HDC, HGLRC);
bool init(string input_file_name);
int  update(void);
bool draw(void);
bool scale_data(void);
void draw_number(float number,bool force_digits=true);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    //TEMP gen data
    //lpCmdLine="test.txt";

    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    //register window class
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "Graph";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);;


    if (!RegisterClassEx(&wcex))
        return 0;

    //create main window
    hwnd = CreateWindowEx(0,
                          "Graph",
                          "Graph",
                          //WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                          WS_POPUP | (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME),
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          g_window_width,
                          g_window_height,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hwnd, nCmdShow);

    //enable OpenGL for the window
    EnableOpenGL(hwnd, &hDC, &hRC);

    //init settings
    if(!init(lpCmdLine)) bQuit=true;

    //program main loop
    while (!bQuit)
    {
        //check for messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            //handle or dispatch messages
            if (msg.message == WM_QUIT)
            {
                bQuit = TRUE;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            update();
            draw();

            SwapBuffers(hDC);
        }
    }

    //shutdown OpenGL
    DisableOpenGL(hwnd, hDC, hRC);

    //destroy the window explicitly
    DestroyWindow(hwnd);

    return msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
        break;

        case WM_DESTROY:
            return 0;

        case WM_MOUSEMOVE:
        {
             g_mouse_pos[0]=LOWORD(lParam);
             g_mouse_pos[1]=HIWORD(lParam);//+g_window_border_shift;

             //reset frameshifter
             g_frame_shift_counter=0;
        }
        break;

        case WM_LBUTTONDOWN:
        {
             g_mouse_but[0]=true;
        }
        break;

        case WM_LBUTTONUP:
        {
             g_mouse_but[0]=false;
        }
        break;

        case WM_RBUTTONDOWN:
        {
             g_mouse_but[1]=true;
        }
        break;

        case WM_RBUTTONUP:
        {
             g_mouse_but[1]=false;
        }
        break;

        case WM_MOUSEWHEEL:
        {
            if(HIWORD(wParam)>5000) {g_mouse_but[2]=true;}
            if(HIWORD(wParam)>100&&HIWORD(wParam)<5000) {g_mouse_but[3]=true;}
        }
        break;

        case WM_KEYDOWN:
        {
            switch (wParam)
            {
                case VK_ESCAPE: PostQuitMessage(0); break;

                case VK_LEFT:
                {
                    if(!g_key_trigger_left)
                    {
                        g_key_trigger_left=true;
                        g_frame_shift_counter-=1;
                    }
                }break;

                case VK_RIGHT:
                {
                    if(!g_key_trigger_right)
                    {
                        g_key_trigger_right=true;
                        g_frame_shift_counter+=1;
                    }
                }break;

                break;
            }
        }
        break;

        case WM_KEYUP:
        {
            switch (wParam)
            {
                case VK_LEFT:
                {
                    g_key_trigger_left=false;
                }break;

                case VK_RIGHT:
                {
                    g_key_trigger_right=false;
                }break;
            }
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC)
{
    PIXELFORMATDESCRIPTOR pfd;

    int iFormat;

    //get the device context (DC)
    *hDC = GetDC(hwnd);

    //set the pixel format for the DC
    ZeroMemory(&pfd, sizeof(pfd));

    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW |
                  PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);

    SetPixelFormat(*hDC, iFormat, &pfd);

    //create and enable the render context (RC) */
    *hRC = wglCreateContext(*hDC);

    wglMakeCurrent(*hDC, *hRC);

    //set 2D mode

    glClearColor(0.0,0.0,0.0,0.0);  //Set the cleared screen colour to black
    glViewport(0,0,g_window_width,g_window_height);   //This sets up the viewport so that the coordinates (0, 0) are at the top left of the window

    //Set up the orthographic projection so that coordinates (0, 0) are in the top left
    //and the minimum and maximum depth is -10 and 10. To enable depth just put in
    //glEnable(GL_DEPTH_TEST)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,g_window_width,g_window_height,0,-1,1);

    //Back to the modelview so we can draw stuff
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //Enable antialiasing
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
}

void DisableOpenGL (HWND hwnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

bool init(string input_file_name)
{
    //open file
    ifstream input_file(input_file_name.c_str());
    if(input_file==0)
    {
        //ERROR file not found
        return false;
    }
    //read data
    string line,word;
    getline(input_file,line);//skip title line
    while(getline(input_file,line))
    {
        st_point new_point1;
        st_point new_point2;
        stringstream ss(line);
        ss>>word;//x
        istringstream(word)>>new_point1.x;
        ss>>word;//y1
        istringstream(word)>>new_point1.y;
        new_point2.x=new_point1.x;
        ss>>word;//y2
        istringstream(word)>>new_point2.y;
        g_vec_points_s1.push_back(new_point1);
        g_vec_points_s2.push_back(new_point2);
        g_vec_points_org_s1.push_back(new_point1);
        g_vec_points_org_s2.push_back(new_point2);
    }

    //scale data
    scale_data();

    //opengl settings
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

int update(void)
{
    //mouse scroll input
    if(g_mouse_but[0]) g_zoom_target_2=false;
    if(g_mouse_but[1]) g_zoom_target_2=true;
    if(g_mouse_but[2])
    {
        if(g_zoom_target_2) g_zoom2_y+=0.01;
        else                g_zoom1_y+=0.01;
        g_mouse_but[2]=false;
        scale_data();
    }
    if(g_mouse_but[3])
    {
        if(g_zoom_target_2) g_zoom2_y-=0.01;
        else                g_zoom1_y-=0.01;
        g_mouse_but[3]=false;
        scale_data();
    }

    //calc number to draw
    if(g_mouse_pos[0]>(g_window_width-g_screen_width)*0.5 && g_mouse_pos[0]<g_screen_width+(g_window_width-g_screen_width)*0.5)
    {
        //find closest x value to cursor
        float value_to_draw_ind=-1;
        for(int i=0;i<(int)g_vec_points_s1.size();i++)
        {
            if(g_vec_points_s1[i].x*g_scale1_x>g_mouse_pos[0]-(g_window_width-g_screen_width)*0.5)
            {
                value_to_draw_ind=i;

                //adjust for frameshifter
                if(g_frame_shift_counter!=0)
                {
                    value_to_draw_ind+=g_frame_shift_counter;
                    //cap
                    if(value_to_draw_ind<0) value_to_draw_ind=0;
                    if(value_to_draw_ind>=(int)g_vec_points_org_s1.size()) value_to_draw_ind=(int)g_vec_points_org_s1.size()-1;
                }

                break;
            }
        }
        if(value_to_draw_ind!=-1 && value_to_draw_ind<(int)g_vec_points_org_s1.size())
        {
            g_value_to_draw_x=g_vec_points_org_s1[value_to_draw_ind].x;
            g_value_to_draw_y=g_vec_points_org_s1[value_to_draw_ind].y;
        }
    }

    return 0;
}

bool draw(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glPushMatrix();
    //window border shift
    glTranslatef(0,g_window_border_shift,0);

    //draw border
    glColor3f(0,0,0);
    glLineWidth(2);
    glBegin(GL_LINE_STRIP);
    glVertex2f((g_window_width-g_screen_width)*0.5,g_window_height-(g_window_height-g_screen_height)*0.5);
    glVertex2f(g_screen_width+(g_window_width-g_screen_width)*0.5,g_window_height-(g_window_height-g_screen_height)*0.5);
    glVertex2f(g_screen_width+(g_window_width-g_screen_width)*0.5,(g_window_height-g_screen_height)*0.5);
    glVertex2f((g_window_width-g_screen_width)*0.5,(g_window_height-g_screen_height)*0.5);
    glVertex2f((g_window_width-g_screen_width)*0.5,g_window_height-(g_window_height-g_screen_height)*0.5);
    glEnd();

    //draw plot1
    glPushMatrix();
    glTranslatef((g_window_width-g_screen_width)*0.5,g_window_height-(g_window_height-g_screen_height)*0.5,0);
    glColor3f(1.0,0.2,0.2);
    glLineWidth(1);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<(int)g_vec_points_s1.size();i++)
    {
        glVertex2f(g_vec_points_s1[i].x*g_scale1_x,-g_vec_points_s1[i].y*g_scale1_y);
    }
    glEnd();
    //draw plot2
    glColor3f(0.2,0.2,1.0);
    glLineWidth(1);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<(int)g_vec_points_s2.size();i++)
    {
        glVertex2f(g_vec_points_s2[i].x*g_scale2_x,-g_vec_points_s2[i].y*g_scale2_y);
    }
    glEnd();
    glPopMatrix();

    //draw vertical marker line
    if(g_mouse_pos[0]>(g_window_width-g_screen_width)*0.5 && g_mouse_pos[0]<g_screen_width+(g_window_width-g_screen_width)*0.5)
    {
        //find closest frame to cursor
        float frame_pos_to_draw_ind=-1;
        for(int i=0;i<(int)g_vec_points_s1.size();i++)
        {
            if(g_vec_points_s1[i].x*g_scale1_x>g_mouse_pos[0]-(g_window_width-g_screen_width)*0.5)
            {
                frame_pos_to_draw_ind=i;

                //adjust for frameshifter
                if(g_frame_shift_counter!=0)
                {
                    frame_pos_to_draw_ind+=g_frame_shift_counter;
                    //cap
                    if(frame_pos_to_draw_ind<0) frame_pos_to_draw_ind=0;
                    if(frame_pos_to_draw_ind>=(int)g_vec_points_org_s1.size()) frame_pos_to_draw_ind=(int)g_vec_points_org_s1.size()-1;
                }

                break;
            }
        }
        if(frame_pos_to_draw_ind!=-1)
        {
            glColor3f(0.4,0.4,0.4);
            glLineWidth(2);
            glBegin(GL_LINES);
            glVertex2f(g_vec_points_org_s1[frame_pos_to_draw_ind].x*g_scale1_x+(g_window_width-g_screen_width)*0.5,
                       g_window_height-(g_window_height-g_screen_height)*0.5);
            glVertex2f(g_vec_points_org_s1[frame_pos_to_draw_ind].x*g_scale1_x+(g_window_width-g_screen_width)*0.5,
                       (g_window_height-g_screen_height)*0.5);
            glEnd();
        }
    }

    //draw diagram values
    if(g_mouse_pos[0]>(g_window_width-g_screen_width)*0.5 && g_mouse_pos[0]<g_screen_width+(g_window_width-g_screen_width)*0.5)
    {
        //draw y value
        glPushMatrix();
        glTranslatef(g_mouse_pos[0]-g_text_scale,g_window_border_shift+4,0);
        draw_number(g_value_to_draw_y);
        glPopMatrix();

        //draw x value
        glPushMatrix();
        glTranslatef(g_mouse_pos[0]-g_text_scale,g_window_height-(g_window_height-g_screen_height)*0.5+4,0);
        draw_number(g_value_to_draw_x,false);
        glPopMatrix();
    }

    glPopMatrix();

    return true;
}

bool scale_data(void)
{
    if(g_vec_points_s1.empty() || g_vec_points_s2.empty()) return false;

    //go through all points
    float x_min=g_vec_points_s1.front().x;
    float y_min=g_vec_points_s1.front().y;
    float x_max=g_vec_points_s1.front().x;
    float y_max=g_vec_points_s1.front().y;
    for(int i=1;i<(int)g_vec_points_s1.size();i++)
    {
        if(g_vec_points_s1[i].x<x_min) x_min=g_vec_points_s1[i].x;
        if(g_vec_points_s1[i].y<y_min) y_min=g_vec_points_s1[i].y;
        if(g_vec_points_s1[i].x>x_max) x_max=g_vec_points_s1[i].x;
        if(g_vec_points_s1[i].y>y_max) y_max=g_vec_points_s1[i].y;
    }
    //shift min values to 0
    for(int i=0;i<(int)g_vec_points_s1.size();i++)
    {
        g_vec_points_s1[i].x-=x_min;
        g_vec_points_s1[i].y-=y_min;
    }
    g_scale1_x=g_screen_width/(x_max-x_min);
    g_scale1_y=g_screen_height/(y_max-y_min)*g_zoom1_y;

    //repeat for second serie
    x_min=g_vec_points_s2.front().x;
    y_min=g_vec_points_s2.front().y;
    x_max=g_vec_points_s2.front().x;
    y_max=g_vec_points_s2.front().y;
    for(int i=1;i<(int)g_vec_points_s2.size();i++)
    {
        if(g_vec_points_s2[i].x<x_min) x_min=g_vec_points_s2[i].x;
        if(g_vec_points_s2[i].y<y_min) y_min=g_vec_points_s2[i].y;
        if(g_vec_points_s2[i].x>x_max) x_max=g_vec_points_s2[i].x;
        if(g_vec_points_s2[i].y>y_max) y_max=g_vec_points_s2[i].y;
    }
    //shift min values to 0
    for(int i=0;i<(int)g_vec_points_s2.size();i++)
    {
        g_vec_points_s2[i].x-=x_min;
        g_vec_points_s2[i].y-=y_min;
    }
    g_scale2_x=g_screen_width/(x_max-x_min);
    g_scale2_y=g_screen_height/(y_max-y_min)*g_zoom2_y;

    return true;
}

void draw_number(float number,bool force_digits)
{
    //convert to string
    stringstream ss;
    ss<<number;
    string string_number(ss.str());
    //force number of digits
    if(force_digits)
    {
        bool dot_found=false;
        for(int i=0;i<(int)string_number.size();i++)
        {
            if(string_number[i]=='.')
            {
                dot_found=true;
                while(i+3>(int)string_number.size()) string_number.append("0");
                break;
            }
        }
        if(!dot_found) string_number.append(".00");
    }

    float scale=g_text_scale;
    float gap_size=g_text_gap_size;

    glPushMatrix();
    glColor4f(0,0,0,0.5);
    glLineWidth(2);
    for(int i=0;i<(int)string_number.size();i++)
    {
        //draw digit
        switch(string_number[i])
        {
            case '0':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(0.0*scale,2.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(0.0*scale,0.0*scale);
                glEnd();
            }break;

            case '1':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.7*scale,2.0*scale);
                glVertex2f(0.7*scale,0.0*scale);
                glEnd();
            }break;

            case '2':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(0.0*scale,2.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glEnd();
            }break;

            case '3':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glVertex2f(0.0*scale,2.0*scale);
                glEnd();
            }break;

            case '4':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glEnd();
            }break;

            case '5':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glVertex2f(0.0*scale,2.0*scale);
                glEnd();
            }break;

            case '6':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(0.0*scale,2.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glEnd();
            }break;

            case '7':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glEnd();
            }break;

            case '8':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(0.0*scale,2.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(1.0*scale,1.0*scale);
                glEnd();
            }break;

            case '9':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(1.0*scale,1.0*scale);
                glVertex2f(0.0*scale,1.0*scale);
                glVertex2f(0.0*scale,0.0*scale);
                glVertex2f(1.0*scale,0.0*scale);
                glVertex2f(1.0*scale,2.0*scale);
                glEnd();
            }break;

            case '.':
            {
                glBegin(GL_LINE_STRIP);
                glVertex2f(0.5*scale,1.9*scale);
                glVertex2f(0.5*scale,2.2*scale);
                glEnd();
            }break;
        }

        //move to next pos
        glTranslatef(scale+gap_size*scale,0,0);
    }
    glPopMatrix();

}
