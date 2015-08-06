# XenAccess Library #

**XenAccess is no longer being maintained.  Please see the [LibVMI](http://www.libvmi.com) project for updated versions**

## Overview ##
When running multiple domains (or virtual machines) using the Xen hypervisor, this library will allow a privileged domain to view the runtime state of another domain. This technique is known as virtual machine introspection. The current software focuses on memory access, but also provides proof-of-concept code for disk monitoring.

## News ##
**5 Jan 2009**: XenAccess version 0.5 is now available!  This release includes greatly improved support for Windows domains, support for newer versions of Xen, lots of bug fixes, support for viewing raw memory files, a new tools directory with adapters for Python and [Volatility](https://www.volatilesystems.com/default/volatility), updated documentation, and much more.  You can download this release using the link on the right side of this page.

**13 Aug 2008**: Help wanted!  The XenAccess project is looking for motivated developers interested in helping to implement new features and improve compatibility across various Xen versions.  Knowledge in C is critical, and experience with Xen and open source projects would be useful too.  Together, we can make XenAccess a true open source alternative to [VMsafe](http://www.vmware.com/overview/security/vmsafe.html)!  In you're interested, contact [Bryan Payne](http://www.bryanpayne.org/4_contact.php).

**17 Apr 2008**: The project has moved to Google, instead of SourceForge.  Over the course of the next few days, we will be completing the migration including moving the mailing list to Google Groups.  Hopefully things didn't break too much in the move, but let us know if you find a problem.

## Xen Compatibility ##
The XenAccess code base requires knowledge of address translations (e.g., between physical and machine addresses) in order to work properly.  Occasionally, Xen makes changes to how these translations are done or to other details that we rely on.  To see if XenAccess is known to work with a particular version of Xen, refer to CompatibleXenVersions, a user maintained wiki page.

## Future Directions ##
The current version of XenAccess provides for memory access between two Linux virtual machines. This is only the first step! Below is a listing (in no particular order) of the features and capabilities that will find their way into XenAccess in the future. If you're a developer and would like to help with this project, this list is a great starting point.
  * Full support for newer versions of Xen
  * Create higher level abstractions (e.g., easier access to memory)
  * Automatic detection of symbol locations

## Background Info & Links ##
  * [XenAccess Paper](http://www.bryanpayne.org/research/papers/acsac07.pdf)(pdf) Our ACSAC 2007 paper.  This is the paper to cite if you use XenAccess for your research project(s).
  * [Virtual Machine Introspection Paper](http://suif.stanford.edu/papers/vmi-ndss03.pdf)(pdf) Building an IDS with introspection.
  * [Copilot Paper](http://www.usenix.org/events/sec04/tech/full_papers/petroni/petroni.pdf)(pdf) Similar technique using a coprocessor instead of a virtual machine.
  * [Xen Project](http://xen.org) A setup with the Xen hypervisor is required to use this library.

## Related Media and Blog Posts ##
  * [McAfee white paper on virtualization security](http://www.mcafee.com/us/local_content/white_papers/wp_avert_virtualization_and_security.zip)
  * [Xen.Org Launches Community Project To Bring VM Introspection to Xen](http://rationalsecurity.typepad.com/blog/2008/10/xenorg-launches-community-project-to-bring-vm-introspection-to-xen.html)
  * [Making Xen virtualization safer with XenAccess](http://news.cnet.com/8301-13505_3-10015925-16.html)
  * [Xen Gets "Safe"](http://telematique.typepad.com/twf/2008/08/xen-gets-safe.html)
  * [An open source project may bring VMsafe capabilities to Xen](http://www.virtualization.info/2008/08/open-source-project-may-bring-vmsafe.html)
  * [XenAccess Projects Gets Wider Promotion](http://blog.xen.org/index.php/2008/08/13/xenaccess-projects-gets-wider-promotion/)
  * [VMSafe for Xen?](http://www.virtuize.com/?p=54)
  * [New Project: XenAccess Library](http://blog.xen.org/index.php/2008/08/08/new-project-xenaccess-library/)

## Contact Information ##
For questions about XenAccess, bug reports, and other related discussion please use the [mailing list](http://groups.google.com/group/xenaccess). For all other matters please contact the project maintainer, [Bryan Payne](http://www.bryanpayne.org/).