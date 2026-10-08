#include <ptgl/Remote/RemotePublisher.h>
int secondTranslationUnit() {
    ptgl::remote::RemotePublisher publisher;
    return publisher.defineView({7,ptgl::remote::ViewKind::scene3D,"Second"})==ptgl::remote::PublishResult::Accepted?7:0;
}
