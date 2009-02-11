/*******************************************************************************#
#           guvcview              http://guvcview.berlios.de                    #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "string_utils.h"
#include "v4l2_controls.h"
#include "v4l2_dyna_ctrls.h"

/* enumerate device controls 
 * args:
 * fd: device file descriptor (must call open on the device first)
 * numb_controls: pointer to integer containing number of existing supported controls
 *
 * returns: allocated list of device controls or NULL on failure                      */
InputControl *
input_enum_controls (int fd, int *num_controls)
{
	int ret=0;
	InputControl * control = NULL;
	int n = 0;
	struct v4l2_queryctrl queryctrl;
	memset(&queryctrl,0,sizeof(struct v4l2_queryctrl));
	int i=0;

	i = V4L2_CID_BASE; /* as defined by V4L2 */
	while (i <= V4L2_CID_LAST_EXTCTR) 
	{ 
		queryctrl.id = i;
		if ((ret=ioctl (fd, VIDIOC_QUERYCTRL, &queryctrl)) == 0 &&
			!(queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)) 
		{
			control = g_renew(InputControl, control, n+1);
			control[n].i = n;
			control[n].id = queryctrl.id;
			control[n].type = queryctrl.type;
			control[n].name = strdup ((char *)queryctrl.name);
			control[n].min = queryctrl.minimum;
			control[n].max = queryctrl.maximum;
			control[n].step = queryctrl.step;
			control[n].default_val = queryctrl.default_value;
			control[n].enabled = (queryctrl.flags & V4L2_CTRL_FLAG_GRABBED) ? 0 : 1;
			control[n].entries = NULL;
			if (queryctrl.type == V4L2_CTRL_TYPE_BOOLEAN)
			{
				control[n].min = 0;
				control[n].max = 1;
				control[n].step = 1;
				/*get the first bit*/
				control[n].default_val=(queryctrl.default_value & 0x0001);
			} 
			else if (queryctrl.type == V4L2_CTRL_TYPE_MENU) 
			{
				struct v4l2_querymenu querymenu;
				memset(&querymenu,0,sizeof(struct v4l2_querymenu));
				control[n].min = 0;

				querymenu.id = queryctrl.id;
				querymenu.index = 0;
				while (ioctl (fd, VIDIOC_QUERYMENU, &querymenu) == 0) 
				{
					control[n].entries = g_renew(pchar, control[n].entries, querymenu.index+1);
					control[n].entries[querymenu.index] = g_strdup ((char *) querymenu.name);
					querymenu.index++;
				}
				control[n].max = querymenu.index - 1;
			}
			n++;
		} 
		else 
		{
			if (errno != EINVAL) g_printerr("Failed to query control id=%d: %s\n"
					, i, strerror(errno));
		}
		i++;
		if (i == V4L2_CID_LAST_NEW)  /* jump between CIDs*/
			i = V4L2_CID_CAMERA_CLASS_BASE_NEW;
		if (i == V4L2_CID_CAMERA_CLASS_LAST)
			i = V4L2_CID_PRIVATE_BASE_OLD;
		if (i == V4L2_CID_PRIVATE_LAST)
			i = V4L2_CID_BASE_EXTCTR;
	}
	*num_controls = n;
	return control;
}

/* free device control list 
 * args:
 * s: pointer to VidState struct containing complete device controls info
 *
 * returns: void*/
void
input_free_controls (struct VidState *s)
{
	int i=0;
	for (i = 0; i < s->num_controls; i++) 
	{
		ControlInfo * ci = s->control_info + i;
		if (ci->widget)
			gtk_widget_destroy (ci->widget);
		if (ci->label)
			gtk_widget_destroy (ci->label);
		if (ci->spinbutton)
			gtk_widget_destroy (ci->spinbutton);
		g_free (s->control[i].name);
		if (s->control[i].type == INPUT_CONTROL_TYPE_MENU) 
		{
			int j;
			for (j = 0; j <= s->control[i].max; j++) 
			{
				g_free (s->control[i].entries[j]);
			}
			g_free (s->control[i].entries);
		}
	}
	g_free (s->control_info);
	s->control_info = NULL;
	g_free (s->control);
	s->control = NULL;
}

/* get device control value
 * args:
 * fd: device file descriptor (must call open on the device first)
 * controls: pointer to InputControl struct containing basic control info
 *
 * returns: control value                                                 */
int
input_get_control (int fd, InputControl * control, int * val)
{
	int ret=0;
	struct v4l2_control c;
	memset(&c,0,sizeof(struct v4l2_control));

	c.id  = control->id;
	ret = ioctl (fd, VIDIOC_G_CTRL, &c);
	if (ret == 0) *val = c.value;
	else perror("VIDIOC_G_CTRL - Unable to get control");
	
	return ret;
}

/* set device control value
 * args:
 * fd: device file descriptor (must call open on the device first)
 * controls: pointer to InputControl struct containing basic control info
 * val: control value 
 *
 * returns: VIDIOC_S_CTRL return value ( failure  < 0 )                   */
int
input_set_control (int fd, InputControl * control, int val)
{
	int ret=0;
	struct v4l2_control c;

	c.id  = control->id;
	c.value = val;
	ret = ioctl (fd, VIDIOC_S_CTRL, &c);
	if (ret < 0) perror("VIDIOC_S_CTRL - Unable to set control");

	return ret;
}

/*--------------------------- focus control ----------------------------------*/
/* get device focus value
 * args:
 * fd: device file descriptor (must call open on the device first)
 *
 * returns: focus value                                                 */
int 
get_focus (int fd)
{
	int ret=0;
	struct v4l2_control c;
	int val=0;

	c.id  = V4L2_CID_FOCUS_LOGITECH;
	ret = ioctl (fd, VIDIOC_G_CTRL, &c);
	if (ret < 0) 
	{
		perror("VIDIOC_G_CTRL - get focus error");
		val = -1;
	}
	else val = c.value;
	
	return val;

}

/* set device focus value
 * args:
 * fd: device file descriptor (must call open on the device first)
 * val: focus value 
 *
 * returns: VIDIOC_S_CTRL return value ( failure  < 0 )                   */
int 
set_focus (int fd, int val) 
{
	int ret=0;
	struct v4l2_control c;

	c.id  = V4L2_CID_FOCUS_LOGITECH;
	c.value = val;
	ret = ioctl (fd, VIDIOC_S_CTRL, &c);
	if (ret < 0) perror("VIDIOC_S_CTRL - set focus error");

	return ret;
}

/* SRC: https://lists.berlios.de/pipermail/linux-uvc-devel/2007-July/001888.html
 * fd: the device file descriptor
 * pan: pan angle in 1/64th of degree
 * tilt: tilt angle in 1/64th of degree
 * reset: set 1 to reset Pan, set 2 to reset tilt, set to 3 to reset pan/tilt to the device origin, set to 0 otherwise 
 *
 * returns: 0 on success or -1 on failure                                                                              */
int uvcPanTilt(int fd, int pan, int tilt, int reset) 
{
	struct v4l2_ext_control xctrls[2];
	struct v4l2_ext_controls ctrls;
	
	if (reset) 
	{
		switch (reset) 
		{
			case 1:
				xctrls[0].id = V4L2_CID_PAN_RESET_NEW;
				xctrls[0].value = 1;
				break;
			
			case 2:
				xctrls[0].id = V4L2_CID_TILT_RESET_NEW;
				xctrls[0].value = 1;
				break;
			
			case 3:
				xctrls[0].value = 3;
				xctrls[0].id = V4L2_CID_PANTILT_RESET_LOGITECH;
				break;
		}
		ctrls.count = 1;
		ctrls.controls = xctrls;
	} 
	else 
	{
		xctrls[0].id = V4L2_CID_PAN_RELATIVE_NEW;
		xctrls[0].value = pan;
		xctrls[1].id = V4L2_CID_TILT_RELATIVE_NEW;
		xctrls[1].value = tilt;
	
		ctrls.count = 2;
		ctrls.controls = xctrls;
	}
	
	if ( ioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls) < 0 ) 
	{
		perror("VIDIOC_S_EXT_CTRLS - Pan/Tilt error\n");
			return -1;
	}
	
	return 0;
}