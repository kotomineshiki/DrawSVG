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

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

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

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  //cout << p.x << "SSS" << point.position.x << endl;
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
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

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
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
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

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
    int sx = (int)floor(x);
    int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= sample_rate * target_w) return;
    if (sy < 0 || sy >= sample_rate * target_h) return;

    // fill sample - NOT doing alpha blending!
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w)] = (uint8_t)(color.r * 255);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 1] = (uint8_t)(color.g * 255);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 2] = (uint8_t)(color.b * 255);
    (sample_buffer)[4 * (sx + sy * sample_rate * target_w) + 3] = (uint8_t)(color.a * 255);

}
void SoftwareRendererImp::rasterize_line(float x0, float y0,
    float x1, float y1,
    Color color) {
    x0 *= sample_rate;
    y0 *= sample_rate;
    x1 *= sample_rate;
    y1 *= sample_rate;

    float delta_y = y1 - y0;
    float delta_x = x1 - x0;
    if (delta_x == 0) {
        if (delta_y > 0) {
            for (int i = y0; i <= y1; i++) {
                rasterize_point(x0, i, color);
            }
        }
        else {
            for (int i = y0; i >= y1; i--) {
                rasterize_point(x0, i, color);
            }
        }
        return;
    }

    float k = delta_y / delta_x;
    if (abs(delta_y) > abs(delta_x) ){
        if (y1 > y0) {
            for (int i = 0; i <= y1 - y0; ++i) {
                rasterize_point(round(i / k + x0), i + y0, color);
            }
        }
        else {
            for (int i = 0; i >= y1 - y0; --i) {
                rasterize_point(round(i / k + x0), i + y0, color);
            }
        }
    }
    else {
        if (x1 > x0) {
            for (int i = 0; i <= x1 - x0; ++i) {
                rasterize_point(i + x0, round(i * k + y0), color);
            }
        }
        else {
            for (int i = 0; i >= x1 - x0; --i) {
                rasterize_point(i + x0, round(i * k + y0), color);
            }
        }


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
            if ((result0 >= 0 && result1 >= 0 && result2 >= 0)) {
                rasterize_point(i, j, color);
            }

        }
    }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 6: 
  // Implement image rasterization

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
// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 4: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 4".
  //  cout << target_w << " " << target_h;
    for (int sx = 0;sx < target_w;++sx) {
        for (int sy = 0;sy < target_h;++sy) {
            Color color = cast_sample_on_pixel(sx, sy);
            render_target[4 * (sx + sy * target_w) + 0] = (uint8_t)(color.r * 255);
            render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
            render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
            render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);
        }
    }
    sample_buffer.assign(sample_buffer.size(), 255);
}


} // namespace CMU462
