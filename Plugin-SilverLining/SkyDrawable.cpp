// Copyright (c) 2008-2012 Sundog Software, LLC. All rights reserved worldwide.

//#******************************************************************************
//#*
//#*      Copyright (C) 2015  Compro Computer Services
//#*      http://openig.compro.net
//#*
//#*      Source available at: https://github.com/CCSI-CSSI/MuseOpenIG
//#*
//#*      This software is released under the LGPL.
//#*
//#*   This software is free software; you can redistribute it and/or modify
//#*   it under the terms of the GNU Lesser General Public License as published
//#*   by the Free Software Foundation; either version 2.1 of the License, or
//#*   (at your option) any later version.
//#*
//#*   This software is distributed in the hope that it will be useful,
//#*   but WITHOUT ANY WARRANTY; without even the implied warranty of
//#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//#*   the GNU Lesser General Public License for more details.
//#*
//#*   You should have received a copy of the GNU Lesser General Public License
//#*   along with this library; if not, write to the Free Software
//#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//#*
//#*    Please direct any questions or comments to the OpenIG Forums
//#*    Email address: openig@compro.net
//#*
//#*
//#*****************************************************************************

#include "SkyDrawable.h"
#include "AtmosphereReference.h"

#include <SilverLining.h>

#include <sstream>

#include <Core-Base/configuration.h>
#include <Core-Base/mathematics.h>

#include <Core-Utils/glerrorutils.h>

#include <Core-PluginBase/plugincontext.h>

#include <Core-OpenIG/openig.h>

#if(__APPLE__)
    #include <OpenGL/gl.h>
    #include <OpenGL/glu.h>
#else
    #include <GL/gl.h>
    #include <GL/glu.h>
#endif

#include <assert.h>

#include <osg/GL2Extensions>
#include <osg/Texture2D>
#include <osg/ValueObject>
#include <osg/io_utils>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <Library-Graphics/OIGMath.h>

#include <Core-Utils/shaderutils.h>

#define UPDATE_DISTANCE_SQ (500.0 * 500.0)
#define CLOUD_SHADOW_TEXTURE 4
#define M_PER_NMI           1852.000001 /* No. of meters in a nautical mile   */

using namespace SilverLining;
using namespace OpenIG::Plugins;

SkyDrawable::SkyDrawable()
        : osg::Drawable()
        , _viewer(0)
        , _skyboxSize(100000)
        , _cloudShadowTexgen(0)
        , _cloudShadowTextureWhiteSubstitute(0)
        , _cloudShadowTextureStage(CLOUD_SHADOW_TEXTURE)
        , _cloudShadowTexgenStage(CLOUD_SHADOW_TEXTURE)
        , _shadowTexHandle(0)
        , _cloudShadowsEnabled(false)
        , _init_shadows_once(false)
        , _needsShadowUpdate(true)
        , _cloudReflections(false)
        , _tz(0)
        , _rainFactor(0.0)
        , _snowFactor(0.0)
        , _removeAllCloudLayers(false)
        , _todDirty(false)
        , _enableCloudShadows(true)
        , _geocentric(false)
        , _slatmosphere(0)
        , _openIG(0)
        , _inCloudsFogDensity(-1.f)
        , _sunMoonBrightnessNight(1.f)
        , _sunMoonBrightnessDay(1.f)
        , cullCallback(0)
        , updateCallback(0)
        , computeBoundingBoxCallback(0)
{
}

SkyDrawable::SkyDrawable(const std::string& path, osgViewer::CompositeViewer* viewer, osg::Light* light, osg::Fog* fog, bool geocentric)
        : osg::Drawable()
        , _viewer(viewer)
        , _skyboxSize(100000)
        , _light(light)
        , _fog(fog)
        , _path(path)
        , _cloudShadowTexgen(0)
        , _cloudShadowTextureWhiteSubstitute(0)
        , _cloudShadowTextureStage(CLOUD_SHADOW_TEXTURE)
        , _cloudShadowTexgenStage(CLOUD_SHADOW_TEXTURE)
        , _shadowTexHandle(0)
        , _todHour(4)
        , _todMinutes(43)
        , _cloudShadowsEnabled(false)
        , _init_shadows_once(false)
        , _needsShadowUpdate(true)
        , _cloudReflections(false)
        , _month(5)
        , _day(23)
        , _year(2016)
        , _rainFactor(0.0)
        , _snowFactor(0.0)
        , _removeAllCloudLayers(false)
        , _todDirty(true)
        , _windSpeed(0.f)
        , _windDirection(0.f)
        , _windDirty(false)
        , _windVolumeHandle(0)
        , _enableCloudShadows(true)
        , _geocentric(geocentric)
        , _slatmosphere(0)
        , _openIG(0)
        , _inCloudsFogDensity(-1.f)
        , _sunMoonBrightnessNight(1.f)
        , _sunMoonBrightnessDay(1.f)
        , cullCallback(0)
        , updateCallback(0)
        , computeBoundingBoxCallback(0)
{
    _cloudShadowTextureStage =  _cloudShadowTexgenStage = OpenIG::Base::Configuration::instance()->getConfig("Clouds-Shadows-Texture-Slot",6);

    const std::string value = OpenIG::Base::Configuration::instance()->getConfig("Enable-Clouds-Shadows","yes");
    _enableCloudShadows =  value == "yes";

    initializeDrawable();
    initializeShadow();
}

void SkyDrawable::initializeShadow()
{
    { // Create white fake texture for use if no cloud layer casting shadows is present
       osg::Image * image = new osg::Image;
       image->allocateImage( 1, 1, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE );
       image->data()[0] = 0xFF;

       _cloudShadowTextureWhiteSubstitute = new osg::Texture2D( image );
       _cloudShadowTextureWhiteSubstitute->setWrap(osg::Texture2D::WRAP_S,osg::Texture2D::REPEAT);
       _cloudShadowTextureWhiteSubstitute->setWrap(osg::Texture2D::WRAP_T,osg::Texture2D::REPEAT);
       _cloudShadowTextureWhiteSubstitute->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
       _cloudShadowTextureWhiteSubstitute->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
    }

    { // Create cloud shadow casting texgen
       _cloudShadowTexgen = new osg::TexGen();
       _cloudShadowTexgen->setMode( osg::TexGen::EYE_LINEAR );
    }

    _cloudShadowCoordMatrixUniform = new osg::Uniform( "cloudShadowCoordMatrix", osg::Matrixf() );

    osgViewer::ViewerBase::Views views;
    _viewer->getViews(views);

    osgViewer::ViewerBase::Views::iterator itr = views.begin();
    for (; itr != views.end(); ++itr)
    {
        osgViewer::View* view = *itr;

        bool openIGScene = false;
        if (view->getUserValue("OpenIG-Scene", openIGScene) && openIGScene)
        {
            view->getSceneData()->getOrCreateStateSet()->addUniform(_cloudShadowCoordMatrixUniform);
            view->getSceneData()->getOrCreateStateSet()->addUniform(new osg::Uniform("cloudShadowTexture", (int)_cloudShadowTextureStage));

            break;
        }
    }
}


void SkyDrawable::initializeDrawable()
{
    setDataVariance(osg::Object::DYNAMIC);
    setUseVertexBufferObjects(false);
    setUseDisplayList(false);

#if 0
    cullCallback = new SilverLiningCullCallback();
    setCullCallback(cullCallback);
#endif

#if 0
    SilverLiningUpdateCallback::Cameras					camerasForUpdate;
    SilverLiningSkyComputeBoundingBoxCallback::Cameras	camerasForComputeBoundingBox;

    osgViewer::ViewerBase::Views views;
    _viewer->getViews(views);

    osgViewer::ViewerBase::Views::iterator itr = views.begin();
    for (; itr != views.end(); ++itr)
    {
        osgViewer::View* view = *itr;

        bool openIGScene = false;
        if (view->getUserValue("OpenIG-Scene", openIGScene) && openIGScene)
        {
            camerasForUpdate.push_back(view->getCamera());
            camerasForComputeBoundingBox.push_back(view->getCamera());
        }
    }

    updateCallback = new SilverLiningUpdateCallback(camerasForUpdate);
    setUpdateCallback(updateCallback);

#if 1
    computeBoundingBoxCallback = new SilverLiningSkyComputeBoundingBoxCallback(camerasForComputeBoundingBox);
    setComputeBoundingBoxCallback(computeBoundingBoxCallback);
#endif

#endif

}

void SkyDrawable::setSunMoonBrightness(float day, float night)
{
    _sunMoonBrightnessDay = day;
    _sunMoonBrightnessNight = night;
}

void SkyDrawable::setLightingParams(OpenIG::Engine* ig)
{
    OpenIG::PluginBase::PluginContext& context = ig->getPluginContext();

    if (_slatmosphere && _light && ig)
    {
        osg::Vec4 diffuse = _light->getDiffuse();
        osg::Vec4 position = _light->getPosition();

        osg::Vec3 sunOrMoonColor;
        _slatmosphere->GetSunOrMoonColor(&sunOrMoonColor.x(), &sunOrMoonColor.y(), &sunOrMoonColor.z());

        sunOrMoonColor.x() = osg::minimum(1.f, sunOrMoonColor.x());
        sunOrMoonColor.y() = osg::minimum(1.f, sunOrMoonColor.y());
        sunOrMoonColor.z() = osg::minimum(1.f, sunOrMoonColor.z());

        osg::Vec3 ambient;
        _slatmosphere->GetAmbientColor(&ambient.x(), &ambient.y(), &ambient.z());
        ambient.x() = osg::minimum(1.f, ambient.x());
        ambient.y() = osg::minimum(1.f, ambient.y());
        ambient.z() = osg::minimum(1.f, ambient.z());

        //std::cout << ambient.x() << ", " << ambient.y() << ", " << ambient.z() << std::endl;

        osg::Vec3 sunOrMoonPosition;
        if (_geocentric)
            _slatmosphere->GetSunPositionGeographic(&sunOrMoonPosition.x(), &sunOrMoonPosition.y(), &sunOrMoonPosition.z());
        else
            _slatmosphere->GetSunOrMoonPosition(&sunOrMoonPosition.x(), &sunOrMoonPosition.y(), &sunOrMoonPosition.z());

        osg::Vec3 horizonColor;
        _slatmosphere->GetHorizonColor(0, &horizonColor.x(), &horizonColor.y(), &horizonColor.z());

        osg::Vec3 fogColor;
        if (_fog)
        {
            fogColor = osg::Vec3(_fog->getColor().x(), _fog->getColor().y(), _fog->getColor().z());
        }

        context.getOrCreateValueObject()->setUserValue("SilverLining-Light-Diffuse", diffuse);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Light-Position", position);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Atmosphere-SunOrMoonColor", sunOrMoonColor);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Atmosphere-SunOrMoonPosition", sunOrMoonPosition);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Atmosphere-Ambient", ambient);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Atmosphere-HorizonColor", horizonColor);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Atmosphere-FogColor", fogColor);
        context.getOrCreateValueObject()->setUserValue("SilverLining-Atmosphere-InClouds-FogDensity", _inCloudsFogDensity);

        //std::cout << "SL position: " << sunOrMoonPosition.x() << "," << sunOrMoonPosition.y() << "," << sunOrMoonPosition.z() << std::endl;
    }
}
void SkyDrawable::resetLighting(SilverLining::Atmosphere *atmosphere) const
{
#if 0
    if (atmosphere)
    {
        atmosphere->GetConditions()->SetFog(_originalInsideFogSettings.a(), _originalInsideFogSettings.r(), _originalInsideFogSettings.g(), _originalInsideFogSettings.b());
    }
#endif
}

osg::Vec3 SkyDrawable::_additionalAmbientLightConstant;
void SkyDrawable::setAdditionalAmbientLightConstant(const osg::Vec3& value)
{
    _additionalAmbientLightConstant = value;
}

osg::Vec3 SkyDrawable::_additionalDiffuseLightConstant;
void SkyDrawable::setAdditionalDiffuseLightConstant(const osg::Vec3& value)
{
    _additionalDiffuseLightConstant = value;
}

void SkyDrawable::setLighting(SilverLining::Atmosphere *atmosphere) const
{
    osg::Light *light = _light;
    osg::Vec4 ambient, diffuse;
    osg::Vec3 direction;
    static bool getDensity = true;
    static float old_density;

    if (atmosphere && light)
    {
        float ra, ga, ba, rd, gd, bd, x, y, z;
        atmosphere->GetAmbientColor(&ra, &ga, &ba);
        atmosphere->GetSunOrMoonColor(&rd, &gd, &bd);
        if (_geocentric)
        {
            atmosphere->GetSunPositionGeographic(&x, &y, &z);
            light->setConstantAttenuation(1.f / 100000000);
        }
        else
        {
            atmosphere->GetSunOrMoonPosition(&x, &y, &z);
        }

        float sunMoonFactor = 1.f;

        direction = osg::Vec3(x, y, z);
        ambient = osg::Vec4(ra, ga, ba, 1.0);
        diffuse = osg::Vec4(rd, gd, bd, 1.0);
        direction.normalize();

        ambient = osg::Vec4(osg::Vec3(ra + _additionalAmbientLightConstant.x(), ga + _additionalAmbientLightConstant.y(), ba + _additionalAmbientLightConstant.z()), 1.0);
        diffuse = osg::Vec4(osg::Vec3(rd + _additionalDiffuseLightConstant.x(), gd + _additionalDiffuseLightConstant.y(), bd + _additionalDiffuseLightConstant.z()), 1.0);

        ambient.r() = osg::minimum(1.f, ambient.r());
        ambient.g() = osg::minimum(1.f, ambient.g());
        ambient.b() = osg::minimum(1.f, ambient.b());

        diffuse.r() = osg::minimum(1.f, diffuse.r());
        diffuse.g() = osg::minimum(1.f, diffuse.g());
        diffuse.b() = osg::minimum(1.f, diffuse.b());

        light->setAmbient(ambient);
        light->setDiffuse(diffuse);
        light->setSpecular(osg::Vec4(1, 1, 1, 1));
        light->setPosition(osg::Vec4(direction.x(), direction.y(), direction.z(), 0));

        float density = 0.f;
        float	hR;
        float	hG;
        float	hB;
        _inCloudsFogDensity = -1.f;

        if(_fog.valid())
        {
            if(atmosphere->GetFogEnabled())
            {//we are inside the stratus layer

                //Save away current density setting so we can put
                //the value back when we exit the cloud layer.
                if(getDensity)
                {
                    old_density = _fog->getDensity();
                    getDensity = false;
                }

                atmosphere->GetFogSettings(&density, &hR, &hG, &hB);
                osg::Vec4 color(hR, hG, hB,1.f);

                _fog->setColor(osg::Vec4(color.r(), color.g(), color.b(), 1.f));
                _fog->setDensity(_inCloudsFogDensity = density);
            }
            else
            {
                //put back original density when we exit the cloud layer.
                if(!getDensity)
                {
                    _fog->setDensity(old_density);
                    getDensity = true;
                }


                atmosphere->GetHorizonColor(0,&hR, &hG, &hB);
                density = _inCloudsFogDensity = _fog->getDensity();
                atmosphere->GetConditions()->SetFog(density,hG,hG,hG);
                _fog->setColor(osg::Vec4(hG,hG,hG,1.f));
                atmosphere->SetHaze(hG,hG,hG,1.5*M_PER_NMI, density);
            }
        }
    }
}

std::string SkyDrawable::_skyModel = "Simple";

void SkyDrawable::setSkyModel(const std::string& skyModel)
{
    _skyModel = skyModel;
}

void SkyDrawable::initializeSilverLining(AtmosphereReference *ar, osg::GLExtensions* ext) const
{
    if (ar && !ar->atmosphereInitialized)
    {
        ar->atmosphereInitialized = true; // only try once.
        SilverLining::Atmosphere *atmosphere = ar->atmosphere;

        if (atmosphere)
        {
//            srand(1234); // constant random seed to ensure consistent clouds across windows

            // Update the path below to where you installed SilverLining's resources folder.
            const char *slPath = getenv("SILVERLINING_PATH");
            if (!slPath)
            {
                slPath = _path.c_str();
                if (!slPath)
                {
#if 0
                    printf("Can't find SilverLining; set the SILVERLINING_PATH environment variable ");
                    printf("to point to the directory containing the SDK.\n");
#else
                    osg::notify(osg::FATAL) << "SilverLining: Can't find SilverLining" << std::endl;
                    osg::notify(osg::FATAL) << "\t Either set the environmental variable SILVERLINING_PATH to point to " << std::endl;
                    osg::notify(osg::FATAL) << "\t the SilverLining installation or set the PATH in the:" << std::endl;
                    osg::notify(osg::FATAL) << "\t      Windows: igplugins\\IgPlugin-SilverLining.dll.xml" << std::endl;
                    osg::notify(osg::FATAL) << "\t      MacOS: /usr/local/lib/igplugins/libIgPlugin-SilverLining.dylib.xml" << std::endl;
                    osg::notify(osg::FATAL) << "\t      Linux: /usr/local/lib/igplugins/libIgPlugin-SilverLining.so.xml" << std::endl;
#endif
                    exit(0);
                }
            }

            std::string resPath(slPath);
#ifdef _WIN32
            resPath += "\\Resources\\";
#else
            resPath += "/Resources/";
#endif
            SL_VECTOR(unsigned int) vectorUserShaders;

            std::stringstream ssLogZShaderVS;
            ssLogZShaderVS<<"vec4 log_z_vs(in vec4 position){ return position; }"<<std::endl;
            GLint logZShaderVS = osg::ShaderUtils::compileShader(ssLogZShaderVS.str(), osg::Shader::VERTEX, ext);
            if (logZShaderVS!=0) vectorUserShaders.push_back(logZShaderVS);

            std::stringstream ssLogZShaderPS;
            ssLogZShaderPS<<"void log_z_ps(float depthoffset){}"<<std::endl;
            GLint logZShaderPS = osg::ShaderUtils::compileShader(ssLogZShaderPS.str(), osg::Shader::FRAGMENT, ext);
            if (logZShaderPS!=0) vectorUserShaders.push_back(logZShaderPS);

            std::stringstream ssBBShader;
            ssBBShader<<"void overrideBillboardFragment_forward_plus_sl_ps(in vec4 texColor, in vec4 lightColor, inout vec4 finalColor){}"<<std::endl;
            GLint BBShader = osg::ShaderUtils::compileShader(ssBBShader.str(), osg::Shader::FRAGMENT, ext);
            if (BBShader!=0) vectorUserShaders.push_back(BBShader);

            srand(1234);

            int ret = atmosphere->Initialize(SilverLining::Atmosphere::OPENGL, resPath.c_str(),
                                             true, 0, vectorUserShaders);
            if (ret != SilverLining::Atmosphere::E_NOERROR)
            {
#if 0
                printf("SilverLining failed to initialize; error code %d.\n", ret);
                printf("Check that the path to the SilverLining installation directory is set properly ");
                printf("in SkyDrawable.cpp (in SkyDrawable::initializeSilverLining)\n");
#else
                osg::notify(osg::FATAL) << "SilverLining: SilverLining failed to initialize; error code " << ret << std::endl;
                osg::notify(osg::FATAL) << "SilverLining: Check the path in:" << std::endl;
                osg::notify(osg::FATAL) << "\t      Windows: igplugins\\IgPlugin-SilverLining.dll.xml" << std::endl;
                osg::notify(osg::FATAL) << "\t      MacOS: /usr/local/lib/igplugins/libIgPlugin-SilverLining.dylib.xml" << std::endl;
                osg::notify(osg::FATAL) << "\t      Linux: /usr/local/lib/igplugins/libIgPlugin-SilverLining.so.xml" << std::endl;
                osg::notify(osg::FATAL) << "\t or the environmental variable SILVERLINING_PATH" << std::endl;
#endif
                exit(0);
            }

            if (_skyModel == "HosekWilkie")
                atmosphere->SetSkyModel(HOSEK_WILKIE);
            else
            if (_skyModel == "Preetham")
                atmosphere->SetSkyModel(PREETHAM);
            else
                atmosphere->SetConfigOption("sky-simple-shader", "yes");

            //atmosphere->SetConfigOption( "render-offscreen", "yes" );
            // This config option must set to get shadow clouds
            // if not set shadow clouds are black and hence overall shadow gets black too
            //atmosphere->SetConfigOption( "cumulus-lighting-quick-and-dirty", "no" );

            // Agreed to not hard code any of the config as long as they are
            // required by some very special effect
#if 0
            atmosphere->SetConfigOption("shadow-map-texture-size", "8192");
            atmosphere->SetConfigOption("enable-precipitation-visibility-effects", "no:");
#endif
            // Let SilverLining know which way is up. OSG usually has Z going up.
            atmosphere->SetUpVector(0, 0, 1);
            atmosphere->SetRightVector(1, 0, 0);

            // Set our location (change this to your own latitude and longitude)
            SilverLining::Location loc;
            loc.SetAltitude(0);
            loc.SetLatitude(_latitude);
            loc.SetLongitude(_longitude);
            atmosphere->GetConditions()->SetLocation(loc);

            atmosphere->EnableLensFlare(true);

            if (cullCallback)
                cullCallback->atmosphere = atmosphere;
        }
    }
}

void SkyDrawable::setShadow(SilverLining::Atmosphere *atmosphere, osg::RenderInfo & renderInfo )
{
    if (!atmosphere || !renderInfo.getState()) return;

    osg::State & state = *renderInfo.getState();

    state.setActiveTextureUnit(_cloudShadowTextureStage);
    _cloudShadowTextureWhiteSubstitute->apply( state );

    // Tell state about our changes
    state.haveAppliedTextureAttribute(_cloudShadowTextureStage, _cloudShadowTextureWhiteSubstitute);

    osg::Matrix cloudShadowProjection;
    cloudShadowProjection.makeIdentity();

    if (_enableCloudShadows && atmosphere->GetShadowMap(_shadowTexHandle, &_lightMVP, &_worldToShadowMapTexCoord, true, 0.1f))
    {

        GLuint shadowMap = (GLuint)(long)(_shadowTexHandle);

        cloudShadowProjection.set( _worldToShadowMapTexCoord.ToArray() );
        cloudShadowProjection = renderInfo.getCurrentCamera()->getInverseViewMatrix() * cloudShadowProjection;

        SkyDrawable* mutableThis = const_cast<SkyDrawable*>(this);

        if (!_texture.valid())
        {
            mutableThis->_texture = new osg::Texture2D;
        }


        osg::ref_ptr<osg::Texture::TextureObject> textureObject = new osg::Texture::TextureObject(_texture.get(),shadowMap,GL_TEXTURE_2D);
        textureObject->setAllocated();

        _texture->setTextureObject(renderInfo.getContextID(),textureObject.get());

        state.setActiveTextureUnit(_cloudShadowTextureStage);
        _texture->apply( state );

        // Tell state about our changes
        state.haveAppliedTextureAttribute(_cloudShadowTextureStage, _texture.get());


    }
    else
    {
        state.setActiveTextureUnit(_cloudShadowTextureStage);
        _cloudShadowTextureWhiteSubstitute->apply( state );

        // Tell state about our changes
        state.haveAppliedTextureAttribute(_cloudShadowTextureStage, _cloudShadowTextureWhiteSubstitute);

    }

    _cloudShadowCoordMatrixUniform->set( cloudShadowProjection );
    _cloudShadowTexgen->setPlanesFromMatrix( cloudShadowProjection );

    // Now goes tricky part of the code !!!
    // We need to interact with OSG during render phase in delicate way and
    // not go out of sync with OpenGL states recorded by osg::State.

    // Change texture stage to proper one
    state.setActiveTextureUnit(_cloudShadowTexgenStage);

    // We set texgen with identity on OpenGL modelview matrix
    // since our TexGen was already premultiplied with inverse view.
    // This method minimizes precision errors as we used OSG double matrices.
    // If we had not premultiplied the texgen then we would need to set
    // view on OpenGL modelview matrix which would cause internal OpenGL
    // premutiplication of TexGen by inverse view using float matrices.
    // Such float computation may cause shadow texture jittering between
    // frames if scenes/terrains spanning large coordinate ranges are used.
    state.applyModelViewMatrix( NULL );

    // Now apply our TexGen
    _cloudShadowTexgen->apply( state );

    state.applyTextureMode(_cloudShadowTexgenStage,GL_TEXTURE_GEN_S,osg::StateAttribute::ON);
    state.applyTextureMode(_cloudShadowTexgenStage,GL_TEXTURE_GEN_T,osg::StateAttribute::ON);
    state.applyTextureMode(_cloudShadowTexgenStage,GL_TEXTURE_GEN_R,osg::StateAttribute::ON);
    state.applyTextureMode(_cloudShadowTexgenStage,GL_TEXTURE_GEN_Q,osg::StateAttribute::ON);

    // Tell state about our changes
    state.haveAppliedTextureAttribute(_cloudShadowTexgenStage, _cloudShadowTexgen);

    // Set this TexGen as a global default
    state.setGlobalDefaultTextureAttribute(_cloudShadowTexgenStage, _cloudShadowTexgen);
}

void SkyDrawable::setTimeOfDay(unsigned int hour, unsigned int minutes)
{
    _todHour = hour;
    _todMinutes = minutes;

    _todDirty = true;
    _utcSet = false;
}

bool SkyDrawable::_utcSet = false;
int SkyDrawable::_utcHour = 12;
int SkyDrawable::_utcMinutes = 0;

void SkyDrawable::setUTC(int hour, int minutes)
{
    _utcHour = hour;
    _utcMinutes = minutes;

    _utcSet = true;
}

void SkyDrawable::setWind(float speed, float direction)
{
    _windSpeed = speed;
    _windDirection = direction;
    _windDirty = true;
}

void SkyDrawable::setVisibility(double visibility)
{
    if (_fog.valid())
    {
        _fog->setDensity(1.0/visibility);
    }
}

void SkyDrawable::addCloudLayer(int id, int type, double altitude, double thickness, double density)
{
    CloudLayersIterator itr = _clouds.find(id);
    if (itr != _clouds.end()) return;

    CloudLayerInfo cli;
    cli._type = type;
    cli._altitude = altitude;
    cli._density = density;
    cli._thickness = thickness;
    cli._id = id;

    _cloudsQueueToAdd.push_back(cli);
}

void SkyDrawable::removeCloudLayer(int id)
{
    CloudLayersIterator itr = _clouds.find(id);
    if (itr == _clouds.end()) return;

    CloudLayerInfo cli;
    cli._id = id;
    cli._handle = itr->second._handle;

    _cloudsQueueToRemove.push_back(cli);

    _clouds.erase(itr);
}

void SkyDrawable::updateCloudLayer(int id, double altitude, double thickness, double density)
{
    CloudLayersIterator itr = _clouds.find(id);
    if (itr == _clouds.end()) return;

    CloudLayerInfo& cli = itr->second;
    if(cli._altitude != abs(altitude * Base::Math::instance()->M_PER_FT))
    {
        osg::notify(osg::NOTICE) << "updateCloudLayer cloud layer alt Now: " << cli._altitude << ", altitudeIn: " << (altitude*Base::Math::instance()->M_PER_FT) << std::endl;
        cli._altitude = abs(altitude * Base::Math::instance()->M_PER_FT);
        cli._dirty = true;
        cli._needReseed = false;
    }

    if (cli._density != density)
    {
        osg::notify(osg::NOTICE) << "updateCloudLayer cloud layer den Now: " << cli._density << ", densityIn: " << density << std::endl;
        cli._density = density;
        cli._dirty = true;
        cli._needReseed = true;
    }

    if (cli._thickness != abs(thickness * Base::Math::instance()->M_PER_FT))
    {
        osg::notify(osg::NOTICE) << "updateCloudLayer cloud layer thk Now: " << cli._thickness << ", thicknessIn: " << (thickness* Base::Math::instance()->M_PER_FT) << std::endl;
        cli._thickness = abs(thickness * Base::Math::instance()->M_PER_FT);
        cli._dirty = true;
        cli._needReseed = true;
    }

    osg::notify(osg::NOTICE) << "updateCloudLayer cloud layer    : " << id << std::endl;
    osg::notify(osg::NOTICE) << "updateCloudLayer cloud layer    : " << cli._type << std::endl;
}

void SkyDrawable::addClouds(SilverLining::Atmosphere *atmosphere, const osg::Vec3d& position)
{

//    SilverLining::CloudLayer *tstLayer = SilverLining::CloudLayerFactory::Create((CloudTypes)4);
//    tstLayer->Restore(*atmosphere,  "/usr/local/muse/amx/data/OsgSceneEffects/CUMULUS_CONGESTUS.config1");
//    if(!tstLayer->GetEnabled())
//        tstLayer->SetEnabled(true);

    CloudLayersQueueIterator itr = _cloudsQueueToAdd.begin();
    for ( ; itr != _cloudsQueueToAdd.end(); ++itr)
    {
        SilverLining::CloudLayer *cloudLayer = SilverLining::CloudLayerFactory::Create((CloudTypes)itr->_type);

        double altitude = itr->_altitude * Base::Math::instance()->M_PER_FT;
        double thickness = itr->_thickness * Base::Math::instance()->M_PER_FT;

        cloudLayer->SetIsInfinite(itr->_infinite);
        cloudLayer->SetBaseAltitude(altitude);
        cloudLayer->SetThickness(thickness);
        cloudLayer->SetDensity(itr->_density);

        // TDB: Read this from config or change the API
        // to support these parameters
        cloudLayer->SetBaseLength(_silverLiningParams.cloudsBaseLength);
        cloudLayer->SetBaseWidth(_silverLiningParams.cloudsBaseWidth);
        //cloudLayer->SetBaseLength(100000);
        //cloudLayer->SetBaseWidth(50000);
        cloudLayer->SetCloudAnimationEffects(0,false);
        cloudLayer->SetLayerPosition(position.x(),-position.y());
        //cloudLayer->SetEnabled(false);

        bool status = cloudLayer->SeedClouds(*atmosphere);
        //cloudLayer->Save("/usr/local/muse/amx/data/OsgSceneEffects/CUMULUS_CONGESTUS.config1");

        //cloudLayer->GenerateShadowMaps(true); //deprecated.....
        int handle = atmosphere->GetConditions()->AddCloudLayer(cloudLayer);
        //status = cloudLayer->Restore(*atmosphere, "/usr/local/muse/amx/data/OsgSceneEffects/CUMULUS_CONGESTUS.config1");

        //int handle = cloudLayer->GetHandle();
        osg::notify(osg::NOTICE) << "add cloud layer    : " << handle << ", status: " << status << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer  id: " << itr->_id << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer typ: " << itr->_type << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer alt: " << altitude << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer thk: " << thickness << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer den: " << itr->_density << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer len: " << _silverLiningParams.cloudsBaseLength << std::endl;
        osg::notify(osg::NOTICE) << "add cloud layer wid: " << _silverLiningParams.cloudsBaseWidth << std::endl;
        //cloudLayer->SetEnabled(true);
        osg::notify(osg::NOTICE) << "add cloud layer enb: " << cloudLayer->GetEnabled() << std::endl;
        double x, y, z;
        cloudLayer->GetLayerPosition(x,y);
        osg::notify(osg::NOTICE) << "add cloud layer pox:y " << x << ':' << -y << std::endl;

        CloudLayerInfo cli = *itr;
        cli._handle = handle;
        cli._thickness = thickness;
        cli._altitude = altitude;

        _clouds[cli._id] = cli;
    }

    _cloudsQueueToAdd.clear();
}

void SkyDrawable::removeClouds(SilverLining::Atmosphere *atmosphere)
{
    CloudLayersQueueIterator itr = _cloudsQueueToRemove.begin();
    for ( ; itr != _cloudsQueueToRemove.end(); ++itr)
    {
        atmosphere->GetConditions()->RemoveCloudLayer(itr->_handle);

        osg::notify(osg::NOTICE) << "removing clouds: " << itr->_handle << std::endl;
    }

    _cloudsQueueToRemove.clear();
}

void SkyDrawable::loadCloudLayer(int id, std::string filename, int type)
{
    CloudLayersIterator itr = _clouds.find(id);
    if (itr != _clouds.end()) return;

    CloudLayerInfo cli;
    cli._id = id;
    cli._filename = filename;
    cli._type = type;

    osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayer( " << id
                             << ", " << filename << ", " << ", type: " << type << ")" << std::endl;
    _cloudFilesQueueToLoad.push_back(cli);
}

void SkyDrawable::loadCloudLayerFiles(SilverLining::Atmosphere *atmosphere, const osg::Vec3d& position)
{
    int handle=-1;
    CloudLayersQueueIterator itr = _cloudFilesQueueToLoad.begin();
    for ( ; itr != _cloudFilesQueueToLoad.end(); ++itr)
    {
        std::string fileToLoad = itr->_filename;
        SilverLining::CloudLayer* tmpLayer =
                SilverLining::CloudLayerFactory::Create( (CloudTypes)itr->_type );

        osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles filename: " << fileToLoad << std::endl;
        std::ifstream infile;
        infile.open(fileToLoad.c_str(), std::ifstream::in);

        if(infile.is_open())
        {
            bool status = tmpLayer->Restore(*atmosphere, fileToLoad.c_str());
            osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles load file status: " << status << std::endl;
            if(!tmpLayer->GetEnabled())
                tmpLayer->SetEnabled(true);
            double x,y;
            tmpLayer->GetLayerPosition(x, y);
            if(x != position.x() || y != -position.y())
            {
                tmpLayer->SetLayerPosition(position.x(), -position.y());
                osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles orig layer pos x: " << x
                                         << ", y: " << y << std::endl;
                osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles setting pos x: " << position.x()
                                         << ", y: " << -position.y() << std::endl;
            }
            handle = atmosphere->GetConditions()->AddCloudLayer(tmpLayer);
            osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles load file handle: " << handle << std::endl;
        }
        else
            osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles unable to load file: " << fileToLoad << std::endl;

        infile.close();

        //Load up our Info data with the data from the layer just loaded so its current.
        CloudLayerInfo cli = *itr;
        cli._handle = tmpLayer->GetHandle();
        cli._altitude = tmpLayer->GetBaseAltitude();
        cli._density = tmpLayer->GetDensity();
        cli._length = tmpLayer->GetBaseLength();
        cli._thickness = tmpLayer->GetThickness();
        cli._type = tmpLayer->GetType();
        cli._width = tmpLayer->GetBaseWidth();

        osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles load file handle: " << cli._handle << std::endl;
        osg::notify(osg::NOTICE) << "SkyDrawable::loadCloudLayerFiles load file   type: " << cli._type << std::endl;

        _clouds[cli._id] = cli;
    }
    _cloudFilesQueueToLoad.clear();
}

void SkyDrawable::setGeocentric(bool geocentric)
{
    _geocentric = geocentric;
}

void SkyDrawable::removeAllCloudLayers()
{
    _removeAllCloudLayers = true;
    _clouds.clear();
}

void SkyDrawable::updateClouds(SilverLining::Atmosphere *atmosphere)
{
    //osg::notify(osg::NOTICE) << "updateclouds()" << std::endl;
    //return;
    CloudLayersIterator itr = _clouds.begin();
    for ( ; itr != _clouds.end(); ++itr)
    {
        CloudLayerInfo& cli= itr->second;

//        osg::notify(osg::NOTICE) << "updating cloud layer    : " << cli._handle << std::endl;

        CloudLayer* cloudLayer = NULL;
        atmosphere->GetConditions()->GetCloudLayer(cli._handle, &cloudLayer);

        if (cloudLayer)
        {
            if(  _silverLiningParams.paramsUpdated && (cloudLayer->GetBaseLength() != _silverLiningParams.cloudsBaseLength
                    || cloudLayer->GetBaseWidth() != _silverLiningParams.cloudsBaseWidth) )
            {
                cli._width = _silverLiningParams.cloudsBaseWidth;
                cli._length = _silverLiningParams.cloudsBaseLength;
                cli._dirty = true;
                cli._needReseed = true;
                _silverLiningParams.paramsUpdated = false;
            }

            if (!cli._dirty) continue;
            cli._dirty = false;

            //We display in feet, but SL wants meters...
            //cli._altitude  *= Base::Math::instance()->M_PER_FT;
            //cli._thickness *= Base::Math::instance()->M_PER_FT;

            //cloudLayer->SetEnabled(false);
            cloudLayer->SetBaseAltitude(cli._altitude);

            if (cli._needReseed)
            {
                osg::notify(osg::NOTICE) << "updating cloud layer reseeding: " << cli._handle << std::endl;
                cli._needReseed = false;
                cloudLayer->SetDensity(cli._density);
                cloudLayer->SetThickness(cli._thickness);
                cloudLayer->SetBaseLength(cli._length);
                cloudLayer->SetBaseWidth(cli._width);

                cloudLayer->SeedClouds(*atmosphere);

                osg::notify(osg::NOTICE) << "updateClouds cloud layer    : " << cli._handle << std::endl;
                osg::notify(osg::NOTICE) << "updateClouds cloud layer tpe: " << cli._type << std::endl;
                osg::notify(osg::NOTICE) << "updateClouds cloud layer alt: " << cli._altitude << std::endl;
                osg::notify(osg::NOTICE) << "updateClouds cloud layer den: " << cli._density << std::endl;
                osg::notify(osg::NOTICE) << "updateClouds cloud layer thk: " << cli._thickness << std::endl;
                osg::notify(osg::NOTICE) << "updateClouds cloud layer len: " << _silverLiningParams.cloudsBaseLength << std::endl;
                osg::notify(osg::NOTICE) << "updateClouds cloud layer wid: " << _silverLiningParams.cloudsBaseWidth << std::endl;

            }
            //cloudLayer->SetEnabled(true);
        }
    }
}

void SkyDrawable::setDate(int month, int day, int year)
{
    _month = month;
    _day = day;
    _year = year;
}

void SkyDrawable::setRain(double factor)
{
    _rainFactor = factor * 30.0;
    //_snowFactor = 0.0;
}

void SkyDrawable::setSnow(double factor)
{
    _snowFactor = factor * 30;
    //_rainFactor = 0.0;
}

double	SkyDrawable::_gamma = 1.8;
bool	SkyDrawable::_userDefinedGamma = false;

void SkyDrawable::setGamma(double gamma, bool use)
{
    _gamma = gamma;
    _userDefinedGamma = use;

}

//We get from this plugins XML datafile now...CGR
double SkyDrawable::_latitude = 12;
double SkyDrawable::_longitude = 42;
void SkyDrawable::setLocation(double lat, double lon)
{
    _latitude = lat;
    _longitude = lon;
}

////We get the data from this plugins XML datafile now...CGR
void SkyDrawable::setTimezone(int tz)
{
    _tz = tz;
}

void SkyDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    SilverLining::Atmosphere *atmosphere = 0;

    AtmosphereReference *ar = dynamic_cast<AtmosphereReference *>(renderInfo.getCurrentCamera()->getUserData());
    if (ar)
    {
        atmosphere = ar->atmosphere;

        SkyDrawable* mutableThis = const_cast<SkyDrawable*>(this);
        mutableThis->_slatmosphere = atmosphere;
    }

    renderInfo.getState()->disableAllVertexArrays();

    if (atmosphere)
    {
        initializeSilverLining(ar, osg::GL2Extensions::Get(renderInfo.getContextID(), true));

        if (_userDefinedGamma) atmosphere->SetGamma(SkyDrawable::_gamma);

        osg::GL2Extensions* ext = osg::GL2Extensions::Get(renderInfo.getContextID(), true);

        GLint program = (GLint)atmosphere->GetSkyShader();
        if(program != 0 && ext != 0)
        {
            ext->glUseProgram(program);
            GLint loc = ext->glGetUniformLocation(program, "ViewOptions_EO");
            if (loc != -1)
            {
                osg::Camera* camera = renderInfo.getCurrentCamera();
                osg::View* view = camera->getView();
                unsigned int options = 0;
                view->getUserValue("Options", options);

                bool POD = options == OpenIG::Engine::EO;

                ext->glUniform1i(loc, POD ? 1 : 0);
            }
            else
                glGetError();

            loc = ext->glGetUniformLocation(program, "ViewOptions_IR");
            if (loc != -1)
            {
                osg::Camera* camera = renderInfo.getCurrentCamera();
                osg::View* view = camera->getView();
                unsigned int options = 0;
                view->getUserValue("Options", options);

                bool POD = options == OpenIG::Engine::IR;

                ext->glUniform1i(loc, POD ? 1 : 0);
            }
            else
                glGetError();

            ext->glUseProgram(0);
        }
//        else
//        {
//            osg::notify(osg::NOTICE) << "SilverLining: Unable to atmosphere->GetSkyShader()!!!!!" << std::endl;
//        }

        atmosphere->GetConditions()->SetPrecipitation(CloudLayer::NONE, 0);
        atmosphere->GetConditions()->SetPrecipitation(CloudLayer::RAIN, _rainFactor);
        atmosphere->GetConditions()->SetPrecipitation(CloudLayer::WET_SNOW, _snowFactor);

        atmosphere->SetCameraMatrix(renderInfo.getCurrentCamera()->getViewMatrix().ptr());
        atmosphere->SetProjectionMatrix(renderInfo.getCurrentCamera()->getProjectionMatrix().ptr());

        SilverLining::LocalTime t;
#if 0
        t.SetFromSystemTime();
#else
        t.SetYear(_year);
        t.SetMonth(_month);
        t.SetDay(_day);
#endif

#if 1
        if (_utcSet)
        {
            t.SetHour(_utcMinutes);
            t.SetMinutes(_utcMinutes);
            t.SetTimeZone(0);
        }
        else
        {
            t.SetHour(_todHour);
            t.SetMinutes(_todMinutes);
            t.SetTimeZone(_tz);
        }
        atmosphere->GetConditions()->SetTime(t);

        SilverLining::Location loc;
        loc.SetAltitude(0);
        loc.SetLatitude(_latitude);
        loc.SetLongitude(_longitude);
        atmosphere->GetConditions()->SetLocation(loc);
#else

#endif

        osg::Vec3d position = renderInfo.getCurrentCamera()->getInverseViewMatrix().getTrans();

        if (_removeAllCloudLayers)
        {
            atmosphere->GetConditions()->RemoveAllCloudLayers();

            SkyDrawable* mutableThis = const_cast<SkyDrawable*>(this);
            mutableThis->_removeAllCloudLayers = false;
        }
        else
        {
            SkyDrawable* mutableThis = const_cast<SkyDrawable*>(this);
            mutableThis->removeClouds(atmosphere);
            mutableThis->addClouds(atmosphere,position);
            mutableThis->loadCloudLayerFiles(atmosphere, position);
            mutableThis->updateClouds(atmosphere);
        }

        if (_windDirty)
        {
            SkyDrawable* mutableThis = const_cast<SkyDrawable*>(this);

            mutableThis->_windDirty = false;

            SilverLining::WindVolume windVolume;
            windVolume.SetWindSpeed(_windSpeed);
            windVolume.SetDirection(_windDirection);

            atmosphere->GetConditions()->RemoveWindVolume(_windVolumeHandle);
            mutableThis->_windVolumeHandle = atmosphere->GetConditions()->SetWind(windVolume);
        }

        if (_geocentric)
        {
            osg::Vec3d ceye;
            osg::Vec3d ccenter;
            osg::Vec3d cup;
            renderInfo.getCurrentCamera()->getViewMatrixAsLookAt(ceye, ccenter, cup);

            osg::Vec3d up = ceye;
            up.normalize();
            osg::Vec3d north = osg::Vec3d(0, 0, 1);
            osg::Vec3d east = north ^ up;
            // Check for edge case of north or south pole
            if (east.length2() == 0) {
                east = osg::Vec3d(1, 0, 0);
            }
            east.normalize();

            atmosphere->SetUpVector(up.x(), up.y(), up.z());
            atmosphere->SetRightVector(east.x(), east.y(), east.z());

            double latitude = 0.0;
            double longitude = 0.0;
            double altitude = 0.0;

            osg::EllipsoidModel em;
            em.convertXYZToLatLongHeight(ceye.x(), ceye.y(), ceye.z(), latitude, longitude, altitude);

            SilverLining::Location loc;
            loc.SetAltitude(altitude);
            loc.SetLongitude(osg::RadiansToDegrees(longitude));
            loc.SetLatitude(osg::RadiansToDegrees(latitude));
            atmosphere->GetConditions()->SetLocation(loc);

            boost::posix_time::ptime t(
                boost::gregorian::date(2015, boost::gregorian::Aug, 2),
                boost::posix_time::hours(_utcSet ? _utcHour : _todHour) +
                boost::posix_time::minutes(_utcSet ? _utcMinutes : _todMinutes)
            );
            boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
            boost::posix_time::time_duration::sec_type x = (t - epoch).total_seconds();

            SilverLining::LocalTime utcTime;
            utcTime.SetFromEpochSeconds(time_t(x));
            utcTime.SetTimeZone(0);
            atmosphere->GetConditions()->SetTime(utcTime);
        }

        double fovy, ar, zNear, zFar;
        renderInfo.getCurrentCamera()->getProjectionMatrixAsPerspective(fovy, ar, zNear, zFar);
        float fCoef = (float)(2.0f / OpenIG::Library::Graphics::Math::Log2(zFar + 1.0f));
        setLogDepthUniforms(atmosphere, renderInfo.getState(), fCoef);

        resetLighting(atmosphere);

        atmosphere->DrawSky(true, _geocentric , _skyboxSize, true, false);

        setLighting(atmosphere);
        const_cast<SkyDrawable*>(this)->setShadow(atmosphere,renderInfo);
        const_cast<SkyDrawable*>(this)->setLightingParams(_openIG);

    }


    renderInfo.getState()->dirtyAllVertexArrays();
}

static void ApplyFCoef(GLint shader, osg::GLExtensions *ext, float fCoef)
{
    if (shader) {
        ext->glUseProgram(shader);
        GLint loc = ext->glGetUniformLocation(shader, "Fcoef");
        if (loc != -1) {
            ext->glUniform1f(loc, fCoef);
        }
    }
}

void SkyDrawable::setLogDepthUniforms(SilverLining::Atmosphere *atmosphere, const osg::State* state, float fCoef) const
{
    if (atmosphere) {
        osg::GLExtensions *ext = osg::GL2Extensions::Get(state->getContextID(), true);
        if (ext) {

            ApplyFCoef(atmosphere->GetSkyShader(), ext, fCoef);
            ApplyFCoef(atmosphere->GetBillboardShader(), ext, fCoef);
            ApplyFCoef(atmosphere->GetPrecipitationShader(), ext, fCoef);
            ApplyFCoef(atmosphere->GetStarShader(), ext, fCoef);
            ApplyFCoef(atmosphere->GetAtmosphericLimbShader(), ext, fCoef);

            SL_VECTOR(unsigned int) cloudShaders = atmosphere->GetActivePlanarCloudShaders();
            SL_VECTOR(unsigned int)::iterator it;
            for (it = cloudShaders.begin(); it != cloudShaders.end(); it++) {
                ApplyFCoef(*it, ext, fCoef);
            }
        }
    }
}

void SilverLiningUpdateCallback::update(osg::NodeVisitor *nv, osg::Drawable* drawable)
{
    SilverLining::Atmosphere *atmosphere = 0;

    osg::Camera* camera = cameras.size() ? cameras.at(0) : 0;
    if (!camera) return;

    AtmosphereReference *ar = dynamic_cast<AtmosphereReference *>(camera->getUserData());
    if (ar) {
        if (!ar->atmosphereInitialized) return;

        atmosphere = ar->atmosphere;
    }

    if (atmosphere) {
        //atmosphere->UpdateSkyAndClouds();
    }

    // Since the skybox bounds are a function of the camera position, always update the bounds.
    drawable->dirtyBound();
}

osg::BoundingBox SilverLiningSkyComputeBoundingBoxCallback::computeBound(const osg::Drawable& drawable) const
{
    osg::BoundingBox box;

    osg::Camera* camera = cameras.size() ? cameras.at(0) : 0;
    if (!camera) return box;

    if (camera)
    {
        const SkyDrawable& skyDrawable = dynamic_cast<const SkyDrawable&>(drawable);

        SilverLining::Atmosphere *atmosphere = 0;
        AtmosphereReference *ar = dynamic_cast<AtmosphereReference *>(camera->getUserData());
        if (ar) {
            atmosphere = ar->atmosphere;
        }

        if (atmosphere)
        {
            double skyboxSize;

            if (skyDrawable.getSkyboxSize() != 0.0) {
                skyboxSize = skyDrawable.getSkyboxSize();
            } else {
                skyboxSize = atmosphere->GetConfigOptionDouble("sky-box-size");
                if (skyboxSize == 0.0) skyboxSize = 1000.0;
            }

            double radius = skyboxSize * 0.5;
            osg::Vec3f eye, center, up;
            camera->getViewMatrixAsLookAt(eye, center, up);
            osg::Vec3d camPos = eye;
            osg::Vec3d min(camPos.x() - radius, camPos.y() - radius, camPos.z() - radius);
            osg::Vec3d max(camPos.x() + radius, camPos.y() + radius, camPos.z() + radius);

            box.set(min, max);

            double dToOrigin = camPos.length();

            bool hasLimb = atmosphere->GetConfigOptionBoolean("enable-atmosphere-from-space");
            if (hasLimb)
            {
                // Compute bounds of atmospheric limb centered at 0,0,0
                double earthRadius = atmosphere->GetConfigOptionDouble("earth-radius-meters");
                double atmosphereHeight = earthRadius +
                    + atmosphere->GetConfigOptionDouble("atmosphere-height");
                double atmosphereThickness = atmosphere->GetConfigOptionDouble("atmosphere-scale-height-meters")
                    + earthRadius;

                osg::BoundingBox atmosphereBox;
                osg::Vec3d atmMin(-atmosphereThickness, -atmosphereThickness, -atmosphereThickness);
                osg::Vec3d atmMax(atmosphereThickness, atmosphereThickness, atmosphereThickness);

                // Expand these bounds by it
                box.expandBy(atmosphereBox);
            }
        }
    }

    return box;
}

