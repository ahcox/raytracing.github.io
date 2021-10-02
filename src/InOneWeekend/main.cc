//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

#include "rtweekend.h"

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

#include <iostream>


color ray_color(const ray& r, const hittable& world, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0,0,0);

    if (world.hit(r, 0.001, infinity, rec)) {
        ray scattered;
        color attenuation;
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth-1);
        return color(0,0,0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*color(1.0, 1.0, 1.0) + t*color(0.5, 0.7, 1.0);
}


hittable_list random_scene() {
    using namespace std;
    hittable_list world;
    std::vector<std::shared_ptr<sphere>> spheres;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    //world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));
    spheres.push_back(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));
    unsigned num_spheres = 0;
    for (int a = -11; a < 11; a += 3){//a++) {
        for (int b = -11; b < 11; b+= 3){ //b++) {

            // The shader starts locking up with too many balls:
            if(num_spheres++ > 120) goto after_loop;

            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    //world.add(make_shared<sphere>(center, 0.2, sphere_material));
                    spheres.push_back(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    //world.add(make_shared<sphere>(center, 0.2, sphere_material));
                    spheres.push_back(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    //world.add(make_shared<sphere>(center, 0.2, sphere_material));
                    spheres.push_back(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }
after_loop:

    auto material1 = make_shared<dielectric>(1.5);
    //world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));
    spheres.push_back(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    //world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));
    spheres.push_back(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    //world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));
    spheres.push_back(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    std::cout.precision(20);
    std::cout << std::defaultfloat;
    std::cout << "// Our scene (a sphere is {x,y,x,radius}):\n";
    std::cout << "const vec4 spheres[] = {\n";

    std::string matref  =     "const MaterialRef sphere_materials[spheres.length()] = {\n";
    std::string lambs   =     "const vec3 lambertian_params[] = {\n";
    std::string mirrors =     "const vec3 mirror_params[] = {\n";
    std::string metals  =     "/// {R,G,B,Fuzziness}\n"
                              "const vec4 metal_params[] = {\n";
    std::string dielectrics = "/// {R,G,B, Index of Refraction}\n"
                               "const vec4 dielectric_params[] = {\n";

    unsigned lamb_i = 0;
    unsigned mirror_i = 0;
    unsigned metal_i = 0;
    unsigned dielectric_i = 0;
    for(const auto sphere : spheres)
    {
        std::cout << "    {" << sphere->center.x() << "f, " << sphere->center.y() << "f, " << sphere->center.z() << "f, " << sphere->radius << "f},\n";
        lambertian* lamb = nullptr;
        metal* met = nullptr;
        dielectric* dia = nullptr;
        if(lamb = dynamic_cast<lambertian*>(&*sphere->mat_ptr)){
            lambs += "    {" + std::to_string(lamb->albedo.x()) + "f, " + std::to_string(lamb->albedo.y()) + "f, " + std::to_string(lamb->albedo.z()) + "f},\n";
            matref += "    {MT_LAMBERTIAN, " + to_string(lamb_i) + "us},\n";
            ++lamb_i;
        }
        else if(met = dynamic_cast<metal*>(&*sphere->mat_ptr)){
            if(met->fuzz == 0){
                mirrors += "    {" + std::to_string(met->albedo.x()) + "f, " + std::to_string(met->albedo.y()) + "f, " + std::to_string(met->albedo.z()) + "f},\n";
                matref += "    {MT_MIRROR, " + to_string(mirror_i++) + "us},\n";
            } else {
                metals += "    {" + std::to_string(met->albedo.x()) + "f, " + std::to_string(met->albedo.y()) + "f, " + std::to_string(met->albedo.z()) + "f, " + std::to_string(met->fuzz) + "f},\n";
                matref += "    {MT_METAL, " + to_string(metal_i++) + "us},\n";
            }
        }
        else if(dia = dynamic_cast<dielectric*>(&*sphere->mat_ptr)){
            dielectrics += "    {1.0f, 1.0f, 1.0f, " + std::to_string(dia->ir) + "f},\n";
            matref += "    {MT_DIELECTRIC, " + to_string(dielectric_i++) + "us},\n";
        }
        else {
            // Unknown material type so reference the first lambertian as a fallback:
            matref += "    {MT_LAMBERTIAN, 0us},\n";
        }
    }

    std::cout << "};\n";

    matref      += "};\n\n";
    lambs       += "};\n\n";
    mirrors     += "};\n\n";
    metals      += "};\n\n";
    dielectrics += "};\n\n";

    std::cout << "\n\n" << matref << lambs << mirrors << metals << dielectrics;

    return world;
}


int main() {

    // Image

    const auto aspect_ratio = 16.0 / 9.0;
    const int image_width = 1200;
    const int image_height = static_cast<int>(image_width / aspect_ratio);
    const int samples_per_pixel = 10;
    const int max_depth = 50;

    // World

    auto world = random_scene();
    exit(1);

    // Camera

    point3 lookfrom(13,2,3);
    point3 lookat(0,0,0);
    vec3 vup(0,1,0);
    auto dist_to_focus = 10.0;
    auto aperture = 0.1;

    camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

    // Render

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    for (int j = image_height-1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0,0,0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width-1);
                auto v = (j + random_double()) / (image_height-1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }
            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }

    std::cerr << "\nDone.\n";
}
