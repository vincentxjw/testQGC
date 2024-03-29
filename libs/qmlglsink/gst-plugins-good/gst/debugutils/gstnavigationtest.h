/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#ifndef __GST_NAVIGATIONTEST_H__
#define __GST_NAVIGATIONTEST_H__

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS
#define GST_TYPE_NAVIGATIONTEST \
  (gst_navigationtest_get_type())
#define GST_NAVIGATIONTEST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_NAVIGATIONTEST,GstNavigationtest))
#define GST_NAVIGATIONTEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_NAVIGATIONTEST,GstNavigationtestClass))
#define GST_IS_NAVIGATIONTEST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_NAVIGATIONTEST))
#define GST_IS_NAVIGATIONTEST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_NAVIGATIONTEST))
typedef struct _GstNavigationtest GstNavigationtest;
typedef struct _GstNavigationtestClass GstNavigationtestClass;

typedef struct
{
  gdouble x;
  gdouble y;
  gint images_left;
  guint8 cy, cu, cv;
} ButtonClick;

struct _GstNavigationtest
{
  GstVideoFilter videofilter;

  gdouble x, y;
  GSList *clicks;
};

struct _GstNavigationtestClass
{
  GstVideoFilterClass parent_class;
};

GType gst_navigationtest_get_type (void);

G_END_DECLS
#endif /* __GST_NAVIGATIONTEST_H__ */
