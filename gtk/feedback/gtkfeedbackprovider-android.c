/*
 * Copyright (c) 2026 Florian "sp1rit" <sp1rit@disroot.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Florian Leander Singer <sp1rit@disroot.org>
 */

#include "config.h"

#include "android/gdkandroidinit-private.h"
#include "android/gdkandroidsurface-private.h"
#include "gtkmodulesprivate.h"
#include "gtknative.h"

#include "gtkfeedbackprovider.h"

struct _GtkFeedbackProviderAndroidClass {
  GtkFeedbackProviderClass parent_class;

  struct {
    jmethodID queue_sound_effect;
    jint sound_effect_click;

    jmethodID perform_haptic_feedback;
    jint feedback_texthandle_move;
  } jcache;
};

struct _GtkFeedbackProviderAndroid {
  GtkFeedbackProvider parent_instance;
};

GDK_DECLARE_INTERNAL_TYPE (GtkFeedbackProviderAndroid, gtk_feedback_provider_android, GTK, FEEDBACK_PROVIDER_ANDROID, GtkFeedbackProvider)
GTK_DEFINE_BUILTIN_MODULE_TYPE_WITH_CODE (GtkFeedbackProviderAndroid, gtk_feedback_provider_android, GTK_TYPE_FEEDBACK_PROVIDER,
                         g_io_extension_point_implement (GTK_FEEDBACK_PROVIDER_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "android",
                                                         10))

static gboolean
gtk_feedback_provider_android_feedback (GtkFeedbackProvider *provider, GdkDevice *dev, GtkFeedback feedback)
{
  GtkFeedbackProviderAndroid *self = (GtkFeedbackProviderAndroid *)provider;
  GtkFeedbackProviderAndroidClass *klass = GTK_FEEDBACK_PROVIDER_ANDROID_GET_CLASS (self);
  GdkSurface *surface = gtk_native_get_surface (gtk_widget_get_native (gtk_feedback_provider_get_widget (provider)));
  if (!GDK_IS_ANDROID_SURFACE (surface))
    return FALSE;
  GdkAndroidSurface *surface_android = GDK_ANDROID_SURFACE (surface);
  if (!surface_android->surface)
    return FALSE;

  JNIEnv *env = gdk_android_get_env ();
  switch (feedback)
    {
    case GTK_FEEDBACK_CLICK:
      (*env)->CallVoidMethod (env, surface_android->surface, klass->jcache.queue_sound_effect, klass->jcache.sound_effect_click);
      return TRUE;
    case GTK_FEEDBACK_TEXTHANDLE_MOVED:
      return (*env)->CallBooleanMethod (env, surface_android->surface, klass->jcache.perform_haptic_feedback, klass->jcache.feedback_texthandle_move);
    default:
      return FALSE;
    }
}

static void
gtk_feedback_provider_android_class_init (GtkFeedbackProviderAndroidClass *klass)
{
  JNIEnv *env;

  GTK_FEEDBACK_PROVIDER_CLASS (klass)->feedback = gtk_feedback_provider_android_feedback;

  env = gdk_android_get_env ();
  (*env)->PushLocalFrame (env, 2);

  // playSoundEffect needs to be run on the Ui thread
  klass->jcache.queue_sound_effect = (*env)->GetMethodID (env, gdk_android_get_java_cache ()->surface.klass, "queueSoundEffect", "(I)V");
  jclass sound_effect_constants = (*env)->FindClass (env, "android/view/SoundEffectConstants");
  jfieldID sound_effect_click = (*env)->GetStaticFieldID (env, sound_effect_constants, "CLICK", "I");
  klass->jcache.sound_effect_click = (*env)->GetStaticIntField (env, sound_effect_constants, sound_effect_click);

  // performHapticFeedback seemingly works wherever
  klass->jcache.perform_haptic_feedback = (*env)->GetMethodID (env, gdk_android_get_java_cache ()->a_view.klass, "performHapticFeedback", "(I)Z");
  jclass haptic_feedback_constants = (*env)->FindClass (env, "android/view/HapticFeedbackConstants");
  jfieldID feedback_texthandle_move = (*env)->GetStaticFieldID (env, haptic_feedback_constants, "TEXT_HANDLE_MOVE", "I");
  klass->jcache.feedback_texthandle_move = (*env)->GetStaticIntField (env, haptic_feedback_constants, feedback_texthandle_move);

  (*env)->PopLocalFrame (env, NULL);
}

static void
gtk_feedback_provider_android_init (GtkFeedbackProviderAndroid *self)
{}
