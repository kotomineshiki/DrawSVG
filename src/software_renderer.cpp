#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {


// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = svg_2_screen;
  while (!transforms.empty()) {
      transforms.pop();
  }
  transforms.push(transformation);
  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transformRelatively(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transformRelatively(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transformRelatively(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transformRelatively(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);
  transforms.pop();
  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  int newSize = sample_rate * sample_rate * target_w * target_h * 4;
  sample_buffer.resize(newSize);
  w = sample_rate * target_w;
  h = sample_rate * target_h;
  sample_buffer.assign(newSize, 255);
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 4: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  int newSize = sample_rate * sample_rate * target_w * target_h * 4;
  sample_buffer.resize(newSize);
  w = sample_rate * width;
  h = sample_rate * height;
  sample_buffer.assign(newSize, 255);
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 5 (part 1):
  // Modify this to implement the transformation stack
    transforms.push(transforms.top() * element->transform);

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }
  transforms.pop();
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transformRelatively(point.position);
  //cout << p.x << "SSS" << point.position.x << endl;
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transformRelatively(line.from);
  Vector2D p1 = transformRelatively(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transformRelatively(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transformRelatively(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transformRelatively(Vector2D(   x   ,   y   ));
  Vector2D p1 = transformRelatively(Vector2D( x + w ,   y   ));
  Vector2D p2 = transformRelatively(Vector2D(   x   , y + h ));
  Vector2D p3 = transformRelatively(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  cout << c.r <<"DD" << c.b << endl;
  if( c.a != 0 ) {
    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p1 = transformRelatively(triangles[i + 0]);
      Vector2D p0 = transformRelatively(triangles[i + 1]);
      Vector2D p2 = transformRelatively(triangles[i + 2]);
      rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
     
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transformRelatively(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transformRelatively(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transformRelatively(image.position);
  Vector2D p1 = transformRelatively(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point(float x, float y, Color color) {
    //xy is the screen-space location of a point
  // fill in the nearest pixel
    int sx = (int)floor(x)*sample_rate;
    int sy = (int)floor(y)*sample_rate;

    // check bounds
    if (sx < 0 || sx >= sample_rate * target_w) return;
    if (sy < 0 || sy >= sample_rate * target_h) return;

    // fill sample - NOT doing alpha blending!
    Color newcolor;
   // newcolor = color * sample_rate * sample_rate;
    //newcolor.a = 1;
    for (int i = sx;i < sx + sample_rate;++i) {
        for (int j = sy;j < sy + sample_rate;++j) {
            fill_sample(i, j,color);
        }
    }
    //fill_sample(sx, sy, newcolor);

}
void SoftwareRendererImp::rasterize_line_helper(float x0, float y0,
    float x1, float y1,
    Color color) {
    float delta_y = y1 - y0;
    float delta_x = x1 - x0;
    if (delta_x == 0) {
        if (delta_y > 0) {
            for (int i = y0; i <= y1; i++) {
                fill_sample(x0, i, color);
            }
        }
        else {
            for (int i = y0; i >= y1; i--) {
                fill_sample(x0, i, color);
            }
        }
        return;
    }

    float k = delta_y / delta_x;
    if (abs(delta_y) > abs(delta_x)) {
        if (y1 > y0) {
            for (int i = 0; i <= y1 - y0; ++i) {
                fill_sample(round(i / k + x0), i + y0, color);
            }
        }
        else {
            for (int i = 0; i >= y1 - y0; --i) {
                fill_sample(round(i / k + x0), i + y0, color);
            }
        }
    }
    else {
        if (x1 > x0) {
            for (int i = 0; i <= x1 - x0; ++i) {
                fill_sample(i + x0, round(i * k + y0), color);
            }
        }
        else {
            for (int i = 0; i >= x1 - x0; --i) {
                fill_sample(i + x0, round(i * k + y0), color);
            }
        }


    }
}
void SoftwareRendererImp::rasterize_line(float x0, float y0,
    float x1, float y1,
    Color color) {
    x0 *= sample_rate;
    y0 *= sample_rate;
    x1 *= sample_rate;
    y1 *= sample_rate;
    for (int i = 0;i < sample_rate;i++) {
        rasterize_line_helper(x0 + i, y0 + i,x1+i,y1+i, color);
    }
    


}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 3: 
  // Implement triangle rasterization
    x0 *= sample_rate;
    y0 *= sample_rate;
    x1 *= sample_rate;
    y1 *= sample_rate;
    x2 *= sample_rate;
    y2 *= sample_rate;

    Vector2D vector0(x0, y0);
    Vector2D vector1(x1, y1);
    Vector2D vector2(x2, y2);
    Vector2D line01 = vector1 - vector0;
    Vector2D line12 = vector2 - vector1;
    Vector2D line20 = vector0 - vector2;
    //dot(line01, line12);
    
    //1 step: get the bounding box
    float xmin = min(x0,min( x1, x2));
    float ymin = min(y0, min(y1, y2));
    float xmax = max(x0,max( x1, x2));
    float ymax = max(y0, max(y1, y2));
    int xmin_int = floor(xmin);
    int ymin_int = floor(ymin);
    int xmax_int = ceil(xmax);
    int ymax_int = ceil(ymax);
    
    for (int i = xmin_int; i <= xmax_int; ++i) {//Todo:take a floor
        for (int j = ymin_int; j <= ymax_int; ++j) {
            Vector2D toCheckPoint(i, j);
            Vector2D line0C = toCheckPoint - vector0;
            Vector2D line1C = toCheckPoint - vector1;
            Vector2D line2C = toCheckPoint - vector2;
            double result0 = cross(line01,line0C);
            double result1 = cross(line12,line1C);
            double result2 = cross(line20,line2C);
            if ((result0 >= 0 && result1 >= 0 && result2 >= 0)||(result0<=0&&result1<=0&&result2<=0)) {//careful about the clockwise
                fill_sample(i, j, color);
            }

        }
    }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization
    x0 *= sample_rate;
    y0 *= sample_rate;
    x1 *= sample_rate;
    y1 *= sample_rate;

    float u, v;
    float u_scale = (x1 - x0) / tex.width;
    float v_scale = (y1 - y0) / tex.height;
    cout << x0 << " " << y0 <<" " << x1 << " " << y1 << endl;
    cout << "CurrentLevelis" << u_scale << endl;
    int sx_min = round(x0);
    int sy_min = round(y0);
    int sx_max = round(x1);
    int sy_max = round(y1);
    float deltay = y1 - y0;
    float deltax = x1 - x0;
    for (int i = sx_min;i <= sx_max;++i) {
        for (int j = sy_min;j <= sy_max;++j) {
            u = (i - x0) / deltax;
            v = (j - y0 )/ deltay;
        //    fill_sample(i, j, sampler->sample_trilinear(tex, u, v,u_scale,v_scale));
            //fill_sample(i, j, sampler->sample_nearest(tex, v,u));
            fill_sample(i, j, sampler->sample_bilinear(tex, v, u));//????what happened to the order of UV
        }
    }


}

Color SoftwareRendererImp::cast_sample_on_pixel(int sx, int sy) {
    //input original Position in Pixel,get the sampled color of that pixel position
    int sample_position_x = sx * sample_rate;
    int sample_position_y = sy * sample_rate;
    
    Color resultColor;
    float divide = (float) 1 / (sample_rate * sample_rate);
    for (int x = sample_position_x;x < sample_position_x + sample_rate;++x) {
        for (int y = sample_position_y;y < sample_position_y + sample_rate;++y) {
            resultColor.r += divide * (sample_buffer)[4 * (x + y * sample_rate * target_w) + 0] / 255;
            resultColor.g += divide * (sample_buffer)[4 * (x + y * sample_rate * target_w) + 1] / 255;
            resultColor.b += divide * (sample_buffer)[4 * (x + y * sample_rate * target_w) + 2] / 255;
            resultColor.a += divide * (sample_buffer)[4 * (x + y * sample_rate * target_w) + 3] / 255;
        }
    }
   // cout << resultColor.r << endl;
    return resultColor;
    //sample_rate
}
void SoftwareRendererImp::fill_sample(int sx, int sy, const Color& color) {
    //int sx = (int)floor(x);
    //int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= sample_rate * target_w) return;
    if (sy < 0 || sy >= sample_rate * target_h) return;

    // fill sample - NOT doing alpha blending!
    float cr = (float)(sample_buffer)[4 * (sx + sy * sample_rate * target_w)]/255.f;
    float cg=(sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 1]/ 255.f;
    float cb=(sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 2]/ 255.f;
    float ca=(sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 3]/ 255.f;
    //if (ca == 1)return;
    Color currentColor(cr, cg, cb, ca);
    Color premul_color(color.r * color.a, color.g * color.a,color.b*color.a,color.a);//premultiplied;
   // Color hovered(premul_color+currentColor*(1-premul_color.a));//Hover
    float nr = premul_color.r + currentColor.r * (1 - premul_color.a);
    float ng = premul_color.g + currentColor.g * (1 - premul_color.a);
    float nb = premul_color.b + currentColor.b * (1 - premul_color.a);
    float na = 1 - (1 - premul_color.a) * (1 - currentColor.a);
    Color hovered(nr,ng,nb,na);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w)] = (uint8_t)(hovered.r * 255);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 1] = (uint8_t)(hovered.g * 255);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 2] = (uint8_t)(hovered.b * 255);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 3] = (uint8_t)(hovered.a * 255);
}
void SoftwareRendererImp::fill_pixel(int x, int y, const Color& color) {
    render_target[4 * (x + y * target_w) + 0] = (uint8_t)(color.r * 255);
    render_target[4 * (x + y * target_w) + 1] = (uint8_t)(color.g * 255);
    render_target[4 * (x + y * target_w) + 2] = (uint8_t)(color.b * 255);
    render_target[4 * (x + y * target_w) + 3] = (uint8_t)(color.a * 255);
}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  //  cout << target_w << " " << target_h;
    for (int sx = 0;sx < target_w;++sx) {
        for (int sy = 0;sy < target_h;++sy) {
            Color color = cast_sample_on_pixel(sx, sy);
            fill_pixel(sx, sy, color);
        }
    }
    sample_buffer.assign(sample_buffer.size(), 255);
}


} // namespace CMU462
